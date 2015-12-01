//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_AES_HPP_INCLUDED
#define JSX_AES_HPP_INCLUDED

#include <vector>

namespace aspect { namespace AES {

/// Maximim cipher text length for a plain text lengh
CORE_API size_t max_cipher_length(size_t plain_length);

/// Maximim plain text length for a cipher text lengh
CORE_API size_t max_plain_length(size_t cipher_length);

/// Encrypt a data[size] with 128-bit key and initialization vector
/// Cipher buffer must be allocated for max_cipher_length(size) bytes
/// Return cipher text length
CORE_API size_t encrypt_128_ecb(void* cipher, void const* data, size_t size,
	void const* key, size_t key_size, void const* iv = nullptr, size_t iv_size = 0);

/// Decrypt a data[size] with 128-bit key and initialization vector
/// Plain buffer must be allocated for max_cipher_length(size) bytes
/// Return plain text length
CORE_API size_t decrypt_128_ecb(void* plain, void const* data, size_t size,
	void const* key, size_t key_size, void const* iv = nullptr, size_t iv_size = 0);

/// Encrypt a data[size] with 128-bit key and initialization vector
inline void encrypt_128_ecb(std::vector<char>& cipher, void const* data, size_t size,
	std::string const& key, std::string const& iv)
{
	cipher.resize(max_cipher_length(size));
	size_t const cipher_len = encrypt_128_ecb(&cipher[0], data, size, key.data(), key.size(), iv.data(), iv.size());
	cipher.resize(cipher_len);
}

/// Decrypt a data[size] with 128-bit key and initialization vector
inline void decrypt_128_ecb(std::vector<char>& plain, void const* data, size_t size,
	std::string const& key, std::string const& iv)
{
	plain.resize(max_plain_length(size));
	size_t const plain_len = decrypt_128_ecb(&plain[0], data, size, key.data(), key.size(), iv.data(), iv.size());
	plain.resize(plain_len);
}

}} // aspect::AES

#endif // JSX_AES_HPP_INCLUDED
