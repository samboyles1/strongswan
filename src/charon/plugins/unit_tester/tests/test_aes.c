/*
 * Copyright (C) 2008 Martin Willi
 * Hochschule fuer Technik Rapperswil
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

#include <library.h>
#include <utils/mutex.h>

#include <unistd.h>
#include <sched.h>
#include <pthread.h>

/**
 * run a test using given values
 */
static bool do_aes_test(u_char *key, int keysize, u_char *iv, 
						u_char *plain, u_char *cipher, int len)
{
	crypter_t *crypter;
	chunk_t enc, dec;
	bool good = TRUE;
	
	crypter = lib->crypto->create_crypter(lib->crypto, ENCR_AES_CBC, keysize);
	if (!crypter)
	{
		return FALSE;
	}
	crypter->set_key(crypter, chunk_create(key, keysize));
	crypter->encrypt(crypter,
					 chunk_create(plain, len), chunk_create(iv, 16), &enc);
	if (!memeq(enc.ptr, cipher, len))
	{
		good = FALSE;
	}
	crypter->decrypt(crypter, enc, chunk_create(iv, 16), &dec);
	if (!memeq(dec.ptr, plain, len))
	{
		good = FALSE;
	}
	free(enc.ptr);
	free(dec.ptr);
	crypter->destroy(crypter);
	return good;
}

/*******************************************************************************
 * AES-128 test
 ******************************************************************************/
bool test_aes128()
{
	/*
	 * Test 1 of RFC3602
	 * Key       : 0x06a9214036b8a15b512e03d534120006
	 * IV        : 0x3dafba429d9eb430b422da802c9fac41
	 * Plaintext : "Single block msg"
	 * Ciphertext: 0xe353779c1079aeb82708942dbe77181a	
	 */
	u_char key1[] = {
		0x06,0xa9,0x21,0x40,0x36,0xb8,0xa1,0x5b,
		0x51,0x2e,0x03,0xd5,0x34,0x12,0x00,0x06
	};
	u_char iv1[] = {
		0x3d,0xaf,0xba,0x42,0x9d,0x9e,0xb4,0x30,
		0xb4,0x22,0xda,0x80,0x2c,0x9f,0xac,0x41
	};
	u_char plain1[] = {
		'S','i','n','g','l','e',' ','b','l','o','c','k',' ','m','s','g'
	};
	u_char cipher1[] = {
		0xe3,0x53,0x77,0x9c,0x10,0x79,0xae,0xb8,
		0x27,0x08,0x94,0x2d,0xbe,0x77,0x18,0x1a
	};
	if (!do_aes_test(key1, 16, iv1, plain1, cipher1, sizeof(plain1)))
	{
		return FALSE;
	}
	
	/*
	 * Test 2 of RFC3602
	 * Key       : 0xc286696d887c9aa0611bbb3e2025a45a
	 * IV        : 0x562e17996d093d28ddb3ba695a2e6f58
	 * Plaintext : 0x000102030405060708090a0b0c0d0e0f
	 *              101112131415161718191a1b1c1d1e1f
	 * Ciphertext: 0xd296cd94c2cccf8a3a863028b5e1dc0a
     *		        7586602d253cfff91b8266bea6d61ab1	
	 */
	u_char key2[] = {
		0xc2,0x86,0x69,0x6d,0x88,0x7c,0x9a,0xa0,
		0x61,0x1b,0xbb,0x3e,0x20,0x25,0xa4,0x5a
	};
	u_char iv2[] = {
		0x56,0x2e,0x17,0x99,0x6d,0x09,0x3d,0x28,
		0xdd,0xb3,0xba,0x69,0x5a,0x2e,0x6f,0x58
	};
	u_char plain2[] = {
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
		0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
		0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
	};
	u_char cipher2[] = {
		0xd2,0x96,0xcd,0x94,0xc2,0xcc,0xcf,0x8a,
		0x3a,0x86,0x30,0x28,0xb5,0xe1,0xdc,0x0a,
		0x75,0x86,0x60,0x2d,0x25,0x3c,0xff,0xf9,
		0x1b,0x82,0x66,0xbe,0xa6,0xd6,0x1a,0xb1
	};
	if (!do_aes_test(key2, 16, iv2, plain2, cipher2, sizeof(plain2)))
	{
		return FALSE;
	}
	 
	/*
	 * Test 3 of RFC3603
	 * Key       : 0x56e47a38c5598974bc46903dba290349
	 * IV        : 0x8ce82eefbea0da3c44699ed7db51b7d9
	 * Plaintext : 0xa0a1a2a3a4a5a6a7a8a9aaabacadaeaf
     *		       b0b1b2b3b4b5b6b7b8b9babbbcbdbebf
     *             c0c1c2c3c4c5c6c7c8c9cacbcccdcecf
     *             d0d1d2d3d4d5d6d7d8d9dadbdcdddedf
	 * Ciphertext: 0xc30e32ffedc0774e6aff6af0869f71aa
     *             0f3af07a9a31a9c684db207eb0ef8e4e
     *             35907aa632c3ffdf868bb7b29d3d46ad
     *             83ce9f9a102ee99d49a53e87f4c3da55	
	 */
	u_char key3[] = {
		0x56,0xe4,0x7a,0x38,0xc5,0x59,0x89,0x74,
		0xbc,0x46,0x90,0x3d,0xba,0x29,0x03,0x49
	};
	u_char iv3[] = {
		0x8c,0xe8,0x2e,0xef,0xbe,0xa0,0xda,0x3c,
		0x44,0x69,0x9e,0xd7,0xdb,0x51,0xb7,0xd9
	};
	u_char plain3[] = {
		0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
		0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
		0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
		0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
		0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
		0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
		0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
		0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf
	};
	u_char cipher3[] = {
		0xc3,0x0e,0x32,0xff,0xed,0xc0,0x77,0x4e,
		0x6a,0xff,0x6a,0xf0,0x86,0x9f,0x71,0xaa,
		0x0f,0x3a,0xf0,0x7a,0x9a,0x31,0xa9,0xc6,
		0x84,0xdb,0x20,0x7e,0xb0,0xef,0x8e,0x4e,
		0x35,0x90,0x7a,0xa6,0x32,0xc3,0xff,0xdf,
		0x86,0x8b,0xb7,0xb2,0x9d,0x3d,0x46,0xad,
		0x83,0xce,0x9f,0x9a,0x10,0x2e,0xe9,0x9d,
		0x49,0xa5,0x3e,0x87,0xf4,0xc3,0xda,0x55
	};
	if (!do_aes_test(key3, 16, iv3, plain3, cipher3, sizeof(plain3)))
	{
		return FALSE;
	}
	return TRUE;
}
