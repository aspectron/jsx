//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#include "jsx/core.hpp"
#include "jsx/aes.hpp"

#include <openssl/aes.h>
#include <openssl/evp.h>

namespace aspect { namespace AES {

size_t max_cipher_length(size_t plain_length) { return plain_length + AES_BLOCK_SIZE; }

size_t max_plain_length(size_t cipher_length) { return cipher_length + AES_BLOCK_SIZE; }

static size_t const KEY_LENGTH = 32;

void fill(uint8_t* buf, void const* key, size_t key_size)
{
	if (!key) key_size = 0;
	if (key_size < KEY_LENGTH) memset(buf + key_size, 0, KEY_LENGTH - key_size);
	if (key_size > 0) memcpy(buf, key, std::min(key_size, KEY_LENGTH));
}

size_t encrypt_128_ecb(void* cipher, void const* data, size_t size,
	void const* key, size_t key_size, void const* iv, size_t iv_size)
{
	uint8_t aes_key[KEY_LENGTH];
	uint8_t aes_iv[KEY_LENGTH];

	fill(aes_key, key, key_size);
	fill(aes_iv, iv, iv_size);

	unsigned char const* plaintext = reinterpret_cast<unsigned char const*>(data);
	int plaintext_len = static_cast<int>(size);

	unsigned char* ciphertext = reinterpret_cast<unsigned char*>(cipher);
	int ciphertext_len = static_cast<int>(max_cipher_length(size));
	int final_len = 0;

	EVP_CIPHER_CTX cipher_ctx;
	EVP_CIPHER_CTX_init(&cipher_ctx);

	EVP_EncryptInit(&cipher_ctx, EVP_aes_128_ecb(), aes_key, aes_iv);
	EVP_EncryptUpdate(&cipher_ctx, ciphertext, &ciphertext_len, plaintext, plaintext_len);
	EVP_EncryptFinal(&cipher_ctx, ciphertext + ciphertext_len, &final_len);

	return ciphertext_len + final_len;
}

size_t decrypt_128_ecb(void* plain, void const* data, size_t size,
	void const* key, size_t key_size, void const* iv, size_t iv_size)
{
	uint8_t aes_key[KEY_LENGTH];
	uint8_t aes_iv[KEY_LENGTH];

	fill(aes_key, key, key_size);
	fill(aes_iv, iv, iv_size);

	unsigned char* plaintext = reinterpret_cast<unsigned char*>(plain);
	int plaintext_len = static_cast<int>(max_plain_length(size));

	unsigned char const* ciphertext = reinterpret_cast<unsigned char const*>(data);
	int ciphertext_len = static_cast<int>(size);
	int final_len = 0;

	EVP_CIPHER_CTX cipher_ctx;
	EVP_CIPHER_CTX_init(&cipher_ctx);

	EVP_DecryptInit(&cipher_ctx, EVP_aes_128_ecb(), aes_key, aes_iv);
	EVP_DecryptUpdate(&cipher_ctx, plaintext, &plaintext_len, ciphertext, ciphertext_len);
	EVP_DecryptFinal(&cipher_ctx, plaintext + plaintext_len, &final_len);

	return plaintext_len + final_len;
}

}} //aspect::AES
