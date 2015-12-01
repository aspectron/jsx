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
#include "jsx/scriptstore.hpp"

#include "jsx/aes.hpp"

namespace aspect {

static size_t const KEY_LENTH = 32;

struct script_header
{
	uint32_t    sign; // == SIGN
	uint8_t     iv[4];
	uint32_t    reserved_[10];
	uint8_t     key[KEY_LENTH];
	uint64_t    data_size;
};

static size_t const SCRIPT_HEADER_SIZE = sizeof(script_header);

static char const SIGN[4] =  { 'j', 's', 'x', '\0' };

static uint8_t const HARMONY_JSX_RIJNDAEL_CRYPT_KEY_0[KEY_LENTH] = "HARMONY JSX RTE 1-0-0.";
static uint8_t const HARMONY_JSX_RIJNDAEL_CRYPT_KEY_1[KEY_LENTH] = "v8 JavaScript Engine. ";

static void init_key(uint8_t* key, uint8_t* iv)
{
	for (uint8_t i = 0; i < KEY_LENTH; ++i)
	{
		key[i] = HARMONY_JSX_RIJNDAEL_CRYPT_KEY_0[i] ^ HARMONY_JSX_RIJNDAEL_CRYPT_KEY_1[i] ^ (i+1);
		iv[i] = 0;
	}
}

static bool is_encrypted(char const* data, size_t size)
{
	return size > sizeof(script_header)
		&& memcmp(data, SIGN, sizeof(SIGN)) == 0;
}

void script_container::encrypt(char const* data, size_t size)
{
	data_.clear();
	src_data_ = data;
	src_size_ = size;

	if (is_encrypted(src_data_, src_size_))
	{
		return;
	}

	uint8_t key[KEY_LENTH];
	uint8_t iv[KEY_LENTH];

	init_key(key, iv);

	data_.resize(SCRIPT_HEADER_SIZE + AES::max_cipher_length(src_size_));

	script_header* header = reinterpret_cast<script_header*>(&data_[0]);
	memcpy(&header->sign, SIGN, sizeof(SIGN));
	header->data_size = src_size_;

	size_t const cipher_size = AES::encrypt_128_ecb(&data_[SCRIPT_HEADER_SIZE],
		&src_data_[0], src_size_, key, sizeof(key), iv, sizeof(iv));
	data_.resize(SCRIPT_HEADER_SIZE + cipher_size);
}

void script_container::decrypt(char const* data, size_t size)
{
	data_.clear();
	src_data_ = data;
	src_size_ = size;

	if (!is_encrypted(src_data_, src_size_))
	{
		return;
	}

	uint8_t key[KEY_LENTH];
	uint8_t iv[KEY_LENTH];

	init_key(key, iv);

	//script_header const* header = reinterpret_cast<script_header const*>(src_data_);

	src_data_ += SCRIPT_HEADER_SIZE;
	src_size_ -= SCRIPT_HEADER_SIZE;

	data_.resize(AES::max_plain_length(src_size_));

	size_t const plain_size = AES::decrypt_128_ecb(&data_[0],
		&src_data_[0], src_size_, key, sizeof(key), iv, sizeof(iv));
	data_.resize(plain_size);
}

} // ::aspect
