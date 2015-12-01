//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_CRYPTO_HPP_INCLUDED
#define JSX_CRYPTO_HPP_INCLUDED

#include "jsx/api.hpp"
#include "jsx/v8_core.hpp"

struct dh_st; // OpenSSL  Diffie-Hellman implementation

namespace aspect { namespace crypto {

void setup_bindings(v8pp::module& bindings);

/// Hash generator
class CORE_API hash_generator : boost::noncopyable
{
public:
	/// Supported hash algorithms
	static std::vector<std::string> algorithms();

	/// Create hash generator for specified algorithm
	/// throws std::invalid_argument on unknown algorithm
	explicit hash_generator(std::string const& algorithm);

	~hash_generator();

	/// Generator hash algorithm
	std::string algorithm() const;

	/// Reset hash generator to initial state
	/// return `this` to chain calls
	hash_generator& reset();

	/// Update generator with new data[size]
	/// return `this` to chain calls
	hash_generator& update(void const* data, size_t size);

	/// Finalize generator
	/// Further update() calls are valid only after reset()
	/// returns hash digest as buffer or hexadecimal-encoded string
	std::string digest(bool hex);

public:
// V8 interface
	explicit hash_generator(v8::FunctionCallbackInfo<v8::Value> const& args);
	void reset_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void update_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void  digest_v8(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	void init(std::string const& algorithm);

	struct context;
	boost::scoped_ptr<context> context_;
};

/// HMAC generator
class CORE_API hmac_generator : boost::noncopyable
{
public:
	/// Supported HMAC algorithms
	static std::vector<std::string> algorithms()
	{
		return hash_generator::algorithms();
	}

	/// Create HMAC generator for specified algorithm and key[key_size]
	/// throws std::invalid_argument on unknown algorithm
	explicit hmac_generator(std::string const& algorithm, void const* key, size_t key_size);

	~hmac_generator();

	/// HMAC generator algorithm
	std::string algorithm() const;

	/// Reset generator  to initial state
	/// return `this` to chain calls
	hmac_generator& reset();

	/// Update generator with new data[size]
	/// return `this` to chain calls
	hmac_generator& update(void const* data, size_t size);

	/// Finalize generator
	/// Further update() calls are valid only after reset()
	/// returns hash digest as buffer or hexadecimal-encoded string
	std::string digest(bool hex);

public:
// V8 interface
	explicit hmac_generator(v8::FunctionCallbackInfo<v8::Value> const& args);
	void reset_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void update_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void  digest_v8(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	void init(std::string const& algorithm, void const* key, size_t key_size);

	struct context;
	boost::scoped_ptr<context> context_;
};

/// Cipher
class CORE_API cipher : boost::noncopyable
{
public:
	/// Supported cipher algorithms
	static std::vector<std::string> algorithms();

	/// Cipher directon
	enum direction { encryption = 1, decryption = 0 };

	/// Create cipher for specified algorithm and directon (either encryption or decryption)
	cipher(std::string const& algorithm, direction dir);
	~cipher();

	/// Cipher algorithm
	std::string algorithm() const;

	/// Cipher direction
	direction dir() const;

	/// Cipher key size
	size_t key_size() const;

	/// Cipher iv size
	size_t iv_size() const;

	/// Cipher block size
	size_t block_size() const;

	/// Set auto padding for the cipher
	void set_auto_padding(bool value);

	/// Is authentication mode cipher
	bool is_auth_mode() const;

	/// Authentication tag availble after final() call
	/// Returns non-empty value only for encryption cipher in authentication mode
	std::vector<char> auth_tag();

	/// Set authentication tag after init()
	/// Allowed for decryption cipher in authentication mode
	bool set_auth_tag(std::vector<char> const& value);

	/// Create key and iv from password and optional salt
	/// Additionally hash algorithm and iterations count may be provided
	void generate_key(std::vector<char>& key, std::vector<char>& iv,
		std::string const& password, std::string const& salt = "",
		unsigned iterations = 0, std::string const& hash_algorithm = "");

	/// Initialize cipher with specified password and optional salt
	/// Additionally hash algorithm and iterations count may be provided
	/// return `this` to chain calls
	cipher& init(std::string const& password, std::string const& salt = "",
		unsigned iterations = 0, std::string const& hash_algorithm = "")
	{
		std::vector<char> key, iv;
		generate_key(key, iv, password, salt, iterations, hash_algorithm);
		return init(key.data(), key.size(), iv.data(), iv.size());
	}

	/// Initialize cipher with specified key and iv
	/// throws std::invalid_argument on unknown algorithm
	/// return `this` to chain calls
	cipher& init(void const* key, size_t key_len, void const* iv, size_t iv_len);

	/// Add data[size]
	/// Should be called only after init()
	/// return `this` to chain calls
	cipher& update(void const* data, size_t size);

	/// Finalize cipher, return reference to the result buffer
	/// Further cipher update() calls are allowed only after init()
	std::vector<char>& final();

public:
// V8 interface
	explicit cipher(v8::FunctionCallbackInfo<v8::Value> const& args);
	void set_auto_padding_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void auth_tag_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void set_auth_tag_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void generate_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void init_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void init_iv_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void update_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void final_v8(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	void init(std::string const& algorithm, direction dir);
	void generate_key_from_v8_args(v8::FunctionCallbackInfo<v8::Value> const& args,
		std::vector<char>& key, std::vector<char>& iv);

	struct context;
	boost::scoped_ptr<context> context_;
	std::vector<char> result_;
};

/// Diffie-Hellman key exchange
class CORE_API diffie_hellman : boost::noncopyable
{
public:
	/// List of predefined Diffie-Hellman group names
	static std::vector<std::string> groups();

	/// Create Diffie-Hellman key exchange and generate a prime with specified bit length and generator value
	explicit diffie_hellman(int prime_length, int generator = 2)
		: impl_()
	{
		reset(prime_length, generator);
	}

	/// Create Diffie-Hellman key exchange with a prime and generator
	explicit diffie_hellman(void const* prime, size_t prime_size, int generator = 2)
		: impl_()
	{
		reset(prime, prime_size, generator);
	}

	/// Create Diffie-Hellman key exchange with a prime and generator
	explicit diffie_hellman(std::vector<char> const& prime, int generator = 2)
		: impl_()
	{
		reset(prime, generator);
	}

	/// Create Diffie-Hellman key exchange with a prime and generator
	explicit diffie_hellman(void const* prime, size_t prime_size, void const* generator, size_t generator_size)
		: impl_()
	{
		reset(prime, prime_size, generator, generator_size);
	}

	/// Create Diffie-Hellman key exchange with a prime and generator
	explicit diffie_hellman(std::vector<char> const& prime, std::vector<char> const& generator)
		: impl_()
	{
		reset(prime, generator);
	}

	/// Create predefined  Diffie-Hellman key exchange
	explicit diffie_hellman(std::string const& group_name)
		: impl_()
	{
		reset(group_name);
	}

	~diffie_hellman();

	/// Generate key pair
	void generate_keys();

	/// Compute and return shared key  using public key from other Diffie-Hellman key exchange
	std::vector<char> compute_shared_key(diffie_hellman const& other_dh);

	/// Compute and return shared key  using other public key
	std::vector<char> compute_shared_key(void const* other_public_key, size_t other_public_key_size);

	/// Compute and return shared key  using other public key
	std::vector<char> compute_shared_key(std::vector<char> const& other_public_key)
	{
		return compute_shared_key(other_public_key.data(), other_public_key.size());
	}

	/// Prime number
	std::vector<char> prime() const;

	/// Generator number
	std::vector<char> generator() const;

	/// Public key
	std::vector<char> public_key() const;

	/// Set new public key
	void set_public_key(void const* data, size_t size);

	/// Set new public key
	void set_public_key(std::vector<char> const& value)
	{
		set_public_key(value.data(), value.size());
	}

	/// Private key
	std::vector<char> private_key() const;

	/// Set new private key
	void set_private_key(void const* data, size_t size);

	/// Set new private key
	void set_private_key(std::vector<char> const& value)
	{
		set_private_key(value.data(), value.size());
	}

	/// Reset Diffie-Hellman key exchange and generate a prime with specified bit length and generator
	void reset(int prime_length, int generator);

	/// Reset Diffie-Hellman key exchange with a prime and generator
	void reset(void const* prime, size_t prime_size, int generator);

	/// Reset Diffie-Hellman key exchange with a prime and generator
	void reset(std::vector<char> const& prime, int generator)
	{
		reset(prime.data(), prime.size(), generator);
	}

	/// Reset Diffie-Hellman key exchange with a prime and generator
	void reset(void const* prime, size_t prime_size, void const* generator, size_t generator_size);

	/// Reset Diffie-Hellman key exchange with a prime and generator
	void reset(std::vector<char> const& prime, std::vector<char> const& generator)
	{
		reset(prime.data(), prime.size(), generator.data(), generator.size());
	}

	/// Reset Diffie-Hellman key exchange to predefined group
	void reset(std::string const& group_name);

public:
// V8 interface
	explicit diffie_hellman(v8::FunctionCallbackInfo<v8::Value> const& args);
	void generate_keys_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void compute_shared_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void prime_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void generator_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void public_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void private_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void set_public_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	void set_private_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args);
	
private:
	void check_impl();

	dh_st* impl_;
};

/// Generate MD5 digest for the buffer data[size], optionally in a hex form
CORE_API std::string md5_digest(void const* data, size_t size, bool hex = false);

/// Generate SHA-1 digest for the buffer data[size], optionally in a hex form
CORE_API std::string sha1_digest(void const* data, size_t size, bool hex = false);

/// Derives a key with specified size from a password  using a salt
/// and iteration count as specified in RFC 2898
CORE_API std::vector<char> pkcs5_pbkdf2_hmac(size_t key_size,
	void const* password, size_t password_size,
	void const* salt = nullptr, size_t salt_size = 0,
	unsigned iterations = 1000, std::string const& hash_algorithm = "sha1");

/// Generate cryptographically strong pseudo-random data of specified size
CORE_API std::vector<char> random_bytes(size_t size);

/// Generate non-cryptographically strong pseudo-random data of specified size
CORE_API std::vector<char> pseudo_random_bytes(size_t size);

}} // aspect::crypto

#endif // JSX_CRYPTO_HPP_INCLUDED
