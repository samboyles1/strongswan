/*
 * Copyright (C) 2006 Martin Willi
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "rekey_child_sa_job.h"

#include <daemon.h>


typedef struct private_rekey_child_sa_job_t private_rekey_child_sa_job_t;

/**
 * Private data of an rekey_child_sa_job_t object.
 */
struct private_rekey_child_sa_job_t {

	/**
	 * Public rekey_child_sa_job_t interface.
	 */
	rekey_child_sa_job_t public;

	/**
	 * protocol of the CHILD_SA (ESP/AH)
	 */
	protocol_id_t protocol;

	/**
	 * inbound SPI of the CHILD_SA
	 */
	uint32_t spi;

	/**
	 * SA destination address
	 */
	host_t *dst;
};

METHOD(job_t, destroy, void,
	private_rekey_child_sa_job_t *this)
{
	this->dst->destroy(this->dst);
	free(this);
}

METHOD(job_t, execute, job_requeue_t,
	private_rekey_child_sa_job_t *this)
{
	ike_sa_t *ike_sa;
	child_cfg_t *old_config;
	child_sa_t *old_child_sa;
	uint64_t packets, bytes;
	rekey_t rekey_policy;
	int rekey_sa = true;

	ike_sa = charon->child_sa_manager->checkout(charon->child_sa_manager,
	                                            this->protocol, this->spi, this->dst, &old_child_sa);

	if (ike_sa == NULL)
	{
		DBG1(DBG_JOB, "CHILD_SA %N/0x%08x/%H not found for rekey",
			 protocol_id_names, this->protocol, htonl(this->spi), this->dst);
	}
	else
	{
		if (ike_sa->get_state(ike_sa) != IKE_PASSIVE)
		{
			old_child_sa->get_usestats(old_child_sa, false, NULL, &bytes, &packets);
			old_config = old_child_sa->get_config(old_child_sa);
			rekey_policy = old_config->get_rekey_policy(old_config);

			if (rekey_policy == REKEY_ON_DEMAND)
			{
				if (bytes == 0 && packets == 0)
				{
					rekey_sa = false;
				}
			}
			else if (rekey_policy == REKEY_NEVER)
			{
				rekey_sa = false;
			}
			else
			{
				rekey_sa = true;
			}

			if (!rekey_sa)
			{
				DBG1 (DBG_JOB, "CHILD_SA not rekeying due to rekey policy");
			}
			else
			{
				DBG1 (DBG_JOB, "CHILD_SA is rekeying now");
				ike_sa->rekey_child_sa(ike_sa, this->protocol, this->spi);
			}
		}
		charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
	}
	return JOB_REQUEUE_NONE;
}

METHOD(job_t, get_priority, job_priority_t,
	private_rekey_child_sa_job_t *this)
{
	return JOB_PRIO_MEDIUM;
}

/*
 * Described in header
 */
rekey_child_sa_job_t *rekey_child_sa_job_create(protocol_id_t protocol,
												uint32_t spi, host_t *dst)
{
	private_rekey_child_sa_job_t *this;

	INIT(this,
		.public = {
			.job_interface = {
				.execute = _execute,
				.get_priority = _get_priority,
				.destroy = _destroy,
			},
		},
		.protocol = protocol,
		.spi = spi,
		.dst = dst->clone(dst),
	);

	return &this->public;
}
