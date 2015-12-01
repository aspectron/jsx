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
#include "jsx/crypto.hpp"

#include "jsx/scriptstore.hpp"
#include "jsx/v8_buffer.hpp"
#include "jsx/utils.hpp"

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/rand.h>

#include "crypto_groups.hpp"

namespace aspect { namespace crypto {

inline EVP_MD const* digestbyname(std::string const& algorithm)
{
	EVP_MD const* md = EVP_get_digestbyname(algorithm.c_str());
	if (!md)
	{
		throw std::invalid_argument("unknown hash algorithm " + algorithm);
	}
	return md;
}

inline EVP_CIPHER const* cipherbyname(std::string const& algorithm)
{
	EVP_CIPHER const* cipher =  EVP_get_cipherbyname(algorithm.c_str());
	if (!cipher)
	{
		throw std::invalid_argument("unknown cipher algorithm " + algorithm);
	}
	return cipher;
}

template<typename T>
static void add_name(T const* descr, char const* from, char const* to, void* arg)
{
	(void)descr; (void)to;

	std::vector<std::string>* result = static_cast<std::vector<std::string>*>(arg);
	_aspect_assert(result);
	result->push_back(from);
}

std::vector<std::string> list_public_key_algorithms()
{
	std::vector<std::string> result;
	for (int i = 0, count = EVP_PKEY_asn1_get_count(); i < count; ++i)
	{
		EVP_PKEY_ASN1_METHOD const* method = EVP_PKEY_asn1_get0(i);
		if (method)
		{
			char const* pem_str;
			EVP_PKEY_asn1_get0_info(nullptr, nullptr, nullptr, nullptr, &pem_str, method);
			if (pem_str)
			{
				result.push_back(pem_str);
			}
		}
	}
	return result;
}

inline void return_buffer(v8::FunctionCallbackInfo<v8::Value> const& args, std::vector<char>& result)
{
	if (result.empty())
	{
		args.GetReturnValue().SetUndefined();
	}
	else
	{
		v8::Isolate* isolate = args.GetIsolate();

		v8_core::buffer* buf = new v8_core::buffer;
		buf->swap(result);
		args.GetReturnValue().Set(v8pp::class_<v8_core::buffer>::import_external(isolate, buf));
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// hash_generator
//
struct hash_generator::context : EVP_MD_CTX
{
	context() { EVP_MD_CTX_init(this); }
	~context() { EVP_MD_CTX_cleanup(this); }
};

std::vector<std::string> hash_generator::algorithms()
{
	std::vector<std::string> result;
	EVP_MD_do_all_sorted(&add_name<EVP_MD>, &result);
	return result;
}

hash_generator::hash_generator(std::string const& algorithm)
{
	init(algorithm);
}

hash_generator::~hash_generator()
{
	context_.reset();
}

std::string hash_generator::algorithm() const
{
	return EVP_MD_name(context_->digest);
}

void hash_generator::init(std::string const& algorithm)
{
	EVP_MD const* md = digestbyname(algorithm);

	context_.reset(new context);
	if (!EVP_DigestInit_ex(context_.get(), md, nullptr))
	{
		throw std::runtime_error("unable to initialize context for hash algorithm " + algorithm);
	}
}

hash_generator& hash_generator::reset()
{
	if (!EVP_DigestInit_ex(context_.get(), context_->digest, nullptr))
	{
		throw std::runtime_error("unable to reset context for hash algorithm " + algorithm());
	}
	return *this;
}

hash_generator& hash_generator::update(void const* data, size_t size)
{
	if (!EVP_DigestUpdate(context_.get(), data, size))
	{
		throw std::runtime_error("unable to update data for hash algorithm " + algorithm());
	}
	return *this;
}

std::string hash_generator::digest(bool hex)
{
	unsigned char result[EVP_MAX_MD_SIZE];
	unsigned result_len;
	if (!EVP_DigestFinal_ex(context_.get(), result, &result_len))
	{
		throw std::runtime_error("unable to finalize data for hash algorithm " + algorithm());
	}
	return hex? utils::hex_str(result, result_len) : std::string(result, result + result_len);
}

hash_generator::hash_generator(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString())
	{
		throw std::invalid_argument("required algorithm string argument");
	}
	
	std::string const algorithm = v8pp::from_v8<std::string>(isolate, args[0]);
	init(algorithm);
}

void hash_generator::reset_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	reset();
	args.GetReturnValue().Set(args.This());
}

void hash_generator::update_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (args[0]->IsString())
	{
		v8::String::Utf8Value const str(args[0]);
		update(*str, str.length());
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		update(buf->data(), buf->size());
	}
	else
	{
		throw std::invalid_argument("required data string or buffer argument");
	}

	args.GetReturnValue().Set(args.This());
}

void hash_generator::digest_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	bool const hex = v8pp::from_v8<bool>(isolate, args[0], true);
	args.GetReturnValue().Set(v8pp::to_v8(isolate, digest(hex)));
}

/////////////////////////////////////////////////////////////////////////////
//
// hmac_generator
//
struct hmac_generator::context : HMAC_CTX
{
	context() { HMAC_CTX_init(this); }
	~context() { HMAC_CTX_cleanup(this); }
};

hmac_generator::hmac_generator(std::string const& algorithm, void const* key, size_t key_size)
{
	init(algorithm, key, key_size);
}

hmac_generator::~hmac_generator()
{
	context_.reset();
}

std::string hmac_generator::algorithm() const
{
	return EVP_MD_name(context_->md);
}

void hmac_generator::init(std::string const& algorithm, void const* key, size_t key_size)
{
	EVP_MD const* md = digestbyname(algorithm);

	context_.reset(new context);
	if (!HMAC_Init_ex(context_.get(), key, static_cast<int>(key_size), md, nullptr))
	{
		throw std::runtime_error("unable to initialize context for HMAC algorithm " + algorithm);
	}
}

hmac_generator& hmac_generator::reset()
{
	if (!HMAC_Init_ex(context_.get(), context_->key, static_cast<int>(context_->key_length), context_->md, nullptr))
	{
		throw std::runtime_error("unable to reset context for HMAC algorithm " + algorithm());
	}
	return *this;
}

hmac_generator& hmac_generator::update(void const* data, size_t size)
{
	if (!HMAC_Update(context_.get(), static_cast<unsigned char const*>(data), size))
	{
		throw std::runtime_error("unable to update data for HMAC algorithm " + algorithm());
	}
	return *this;
}

std::string hmac_generator::digest(bool hex)
{
	unsigned char result[EVP_MAX_MD_SIZE];
	unsigned result_len;
	if (!HMAC_Final(context_.get(), result, &result_len))
	{
		throw std::runtime_error("unable to finalize data for HMAC algorithm " + algorithm());
	}
	return hex? utils::hex_str(result, result_len) : std::string(result, result + result_len);
}

hmac_generator::hmac_generator(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString())
	{
		throw std::invalid_argument("required algorithm string for 1st argument");
	}

	std::string const algorithm = v8pp::from_v8<std::string>(isolate, args[0]);

	boost::scoped_ptr<v8::String::Utf8Value> key_str;
	char const* key = nullptr;
	size_t key_size = 0;
	if (args[1]->IsString())
	{
		// store key in Utf8 string key_str
		key_str.reset(new v8::String::Utf8Value(args[1]));
		key = **key_str;
		key_size = key_str->length();
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[1]))
	{
		key = buf->data();
		key_size = buf->size();
	}
	else
	{
		throw std::invalid_argument("required key string or buffer for 2nd argument");
	}

	init(algorithm, key, key_size);
}

void hmac_generator::reset_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	reset();
	args.GetReturnValue().Set(args.This());
}

void hmac_generator::update_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (args[0]->IsString())
	{
		v8::String::Utf8Value const str(args[0]);
		update(*str, str.length());
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		update(buf->data(), buf->size());
	}
	else
	{
		throw std::invalid_argument("required data string or buffer argument");
	}

	args.GetReturnValue().Set(args.This());
}

void hmac_generator::digest_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	bool const hex = v8pp::from_v8<bool>(isolate, args[0], true);
	args.GetReturnValue().Set(v8pp::to_v8(isolate, digest(hex)));
}

/////////////////////////////////////////////////////////////////////////////
//
// cipher
//
std::vector<std::string> cipher::algorithms()
{
	std::vector<std::string> result;
	EVP_CIPHER_do_all_sorted(add_name<EVP_CIPHER>, &result);
	return result;
}

struct cipher::context : EVP_CIPHER_CTX
{
	context() { EVP_CIPHER_CTX_init(this); }
	~context() { EVP_CIPHER_CTX_cleanup(this); }
};

cipher::cipher(std::string const& algorithm, direction dir)
{
	init(algorithm, dir);
}

cipher::~cipher()
{
	context_.reset();
}

std::string cipher::algorithm() const
{
	return EVP_CIPHER_name(context_->cipher);
}

cipher::direction cipher::dir() const
{
	return context_->encrypt? encryption : decryption;
}

size_t cipher::key_size() const
{
	return EVP_CIPHER_CTX_key_length(context_.get());
}

size_t cipher::iv_size() const
{
	return EVP_CIPHER_CTX_iv_length(context_.get());
}

size_t cipher::block_size() const
{
	return EVP_CIPHER_CTX_block_size(context_.get());
}

void cipher::set_auto_padding(bool value)
{
	EVP_CIPHER_CTX_set_padding(context_.get(), value);
}

bool cipher::is_auth_mode() const
{
	return EVP_CIPHER_mode(context_->cipher) == EVP_CIPH_GCM_MODE;
}

std::vector<char> cipher::auth_tag()
{
	std::vector<char> result;

	if (is_auth_mode() && dir() == encryption)
	{
		result.resize(EVP_GCM_TLS_TAG_LEN);
		if (!EVP_CIPHER_CTX_ctrl(context_.get(), EVP_CTRL_GCM_GET_TAG, EVP_GCM_TLS_TAG_LEN, &result[0]))
		{
			result.clear();
		}
	}
	return result;
}

bool cipher::set_auth_tag(std::vector<char> const& value)
{
	int const length = static_cast<int>(value.size());
	void* data = length? const_cast<char*>(value.data()) : nullptr;

	return is_auth_mode() && dir() == decryption
		&& EVP_CIPHER_CTX_ctrl(context_.get(), EVP_CTRL_GCM_SET_TAG, length, data);
}

void cipher::generate_key(std::vector<char>& key, std::vector<char>& iv,
		std::string const& password, std::string const& salt,
		unsigned iterations, std::string const& hash_algorithm)
{
	EVP_MD const* md = hash_algorithm.empty()? EVP_md5() : digestbyname(hash_algorithm);

	unsigned char salt_buf[PKCS5_SALT_LEN] = {};
	memcpy(salt_buf, salt.data(), std::min(salt.size(), sizeof(salt_buf)));

	key.resize(EVP_MAX_KEY_LENGTH);
	iv.resize(EVP_MAX_IV_LENGTH);
	
	if (!EVP_BytesToKey(context_->cipher, md, salt.empty()? nullptr : salt_buf,
		reinterpret_cast<unsigned char const*>(password.data()), static_cast<int>(password.size()),
		iterations? iterations : PKCS5_DEFAULT_ITER,
		reinterpret_cast<unsigned char*>(key.data()), reinterpret_cast<unsigned char*>(iv.data())))
	{
		throw std::runtime_error("unable to generate key for cipher algorithm " + algorithm());
	}

	key.resize(key_size());
	iv.resize(iv_size());
}

void cipher::init(std::string const& algorithm, direction dir)
{
	EVP_CIPHER const* type = cipherbyname(algorithm);

	context_.reset(new context);
	if (!EVP_CipherInit(context_.get(), type, nullptr, nullptr, dir))
	{
		throw std::runtime_error("unable to initialize context for cipher algorithm " + algorithm);
	}
}

cipher& cipher::init(void const* key, size_t key_len, void const* iv, size_t iv_len)
{
	if (key_size() != key_len
		&& !EVP_CIPHER_CTX_set_key_length(context_.get(), static_cast<int>(key_len)))
	{
		throw std::invalid_argument("invalid key length for algorithm " + algorithm());
	}

	if (iv_size() != iv_len && !(EVP_CIPHER_CTX_mode(context_.get()) & EVP_CIPH_ECB_MODE))
	{
		throw std::invalid_argument("invalid iv length for algorithm " + algorithm());
	}

	if (!EVP_CipherInit(context_.get(), context_->cipher,
		static_cast<unsigned char const*>(key), static_cast<unsigned char const*>(iv), dir()))
	{
		throw std::runtime_error("unable to set key and iv for cipher algorithm " + algorithm());
	}

	result_.clear();
	return *this;
}

cipher& cipher::update(void const* data, size_t size)
{
	unsigned char const* in = reinterpret_cast<unsigned char const*>(data);
	int in_len = static_cast<int>(size);

	size_t const prev_size = result_.size();
	int out_len = static_cast<int>(size + block_size());

	result_.resize(prev_size + out_len);
	unsigned char* out = reinterpret_cast<unsigned char*>(&result_[0] + prev_size);

	if (!EVP_CipherUpdate(context_.get(), out, &out_len, in, in_len))
	{
		throw std::runtime_error("unable to update data for cipher algorithm " + algorithm());
	}
	result_.resize(prev_size + out_len);

	return *this;
}

std::vector<char>& cipher::final()
{
	size_t const prev_size = result_.size();
	int out_len = static_cast<int>(block_size());

	result_.resize(prev_size + out_len);
	unsigned char* out = reinterpret_cast<unsigned char*>(&result_[0] + prev_size);

	if (!EVP_CipherFinal_ex(context_.get(), out, &out_len))
	{
		char const* const reason = is_auth_mode()?
			"unable to authenticate data for cipher algorithm " :
			"unable to finalize data for cipher algorithm ";

		throw std::runtime_error(reason + algorithm());
	}
	result_.resize(prev_size + out_len);

	return result_;
}

cipher::cipher(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString())
	{
		throw std::invalid_argument("required algorithm string");
	}

	std::string const algorithm = v8pp::from_v8<std::string>(isolate, args[0]);
	direction const dir = v8pp::from_v8<direction>(isolate, args[1]->ToNumber(), cipher::encryption);

	init(algorithm, dir);
}

void cipher::set_auto_padding_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	bool const auto_padding = v8pp::from_v8<bool>(isolate, args[0], true);
	set_auto_padding(auto_padding);
}

void cipher::auth_tag_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char> result = auth_tag();
	return_buffer(args, result);
}

void cipher::set_auth_tag_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	
	v8_core::buffer* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]);
	if (!buf)
	{
		throw std::invalid_argument("buffer argument required");
	}

	std::vector<char> tag;
	buf->swap(tag);

	args.GetReturnValue().Set(set_auth_tag(tag));
}

void cipher::generate_key_from_v8_args(v8::FunctionCallbackInfo<v8::Value> const& args,
		std::vector<char>& key, std::vector<char>& iv)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Value> password_arg = args[0], salt_arg = args[1], iterations_arg = args[2], hash_arg = args[3];

	std::string password, salt, hash_algorithm;
	unsigned iterations = 0;

	// 1st argument can be a string, buffer or options object
	for (int i = 0; i < 2; ++i)
	{
		if (password_arg->IsString())
		{
			password = v8pp::from_v8<std::string>(isolate, password_arg);
			break;
		}
		else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, password_arg))
		{
			password.assign(buf->data(), buf->data() + buf->size());
			break;
		}
		else if (password_arg->IsObject() && i == 0)
		{
			v8::Local<v8::Object> options = args[0].As<v8::Object>();
			get_option(isolate, options, "password", password_arg);
			get_option(isolate, options, "salt", salt_arg);
			get_option(isolate, options, "iterations", iterations_arg);
			get_option(isolate, options, "hash", hash_arg);
		}
		else
		{
			throw std::invalid_argument("require password string or buffer or options object 1st argument");
		}
	}

	if (salt_arg->IsString())
	{
		salt = v8pp::from_v8<std::string>(isolate, salt_arg);
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, salt_arg))
	{
		salt.assign(buf->data(), buf->data() + buf->size());
	}
	else if (!(salt_arg.IsEmpty() || salt_arg->IsUndefined()))
	{
		throw std::invalid_argument("salt argument should be a string or buffer");
	}

	if (iterations_arg->IsNumber())
	{
		iterations = v8pp::from_v8<unsigned>(isolate, iterations_arg);
	}
	else if (!(iterations_arg.IsEmpty() || iterations_arg->IsUndefined()))
	{
		throw std::invalid_argument("iterations argument should be a number");
	}

	if (hash_arg->IsString())
	{
		hash_algorithm = v8pp::from_v8<std::string>(isolate, hash_arg);
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, hash_arg))
	{
		hash_algorithm.assign(buf->data(), buf->data() + buf->size());
	}
	else if (!(hash_arg.IsEmpty() || hash_arg->IsUndefined()))
	{
		throw std::invalid_argument("hash argument should be a string or buffer");
	}

	generate_key(key, iv, password, salt, iterations, hash_algorithm);
}

void cipher::generate_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char> key, iv;
	generate_key_from_v8_args(args, key, iv);

	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8_core::buffer* key_buf = new v8_core::buffer;
	key_buf->swap(key);

	v8_core::buffer* iv_buf = new v8_core::buffer;
	iv_buf->swap(iv);

	v8::Local<v8::Object> result = v8::Object::New(isolate);
	set_option(isolate, result, "key", v8pp::class_<v8_core::buffer>::import_external(isolate, key_buf));
	set_option(isolate, result, "iv", v8pp::class_<v8_core::buffer>::import_external(isolate, iv_buf));

	args.GetReturnValue().Set(scope.Escape(result));
}

void cipher::init_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char> key, iv;

	generate_key_from_v8_args(args, key, iv);
	init(key.data(), key.size(), iv.data(), iv.size());

	args.GetReturnValue().Set(args.This());
}

void cipher::init_iv_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	boost::scoped_ptr<v8::String::Utf8Value> key_str;
	char const* key = nullptr;
	size_t key_size = 0;
	if (args[0]->IsString())
	{
		// store key in Utf8 string key_str
		key_str.reset(new v8::String::Utf8Value(args[0]));
		key = **key_str;
		key_size = key_str->length();
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		key = buf->data();
		key_size = buf->size();
	}
	else
	{
		throw std::invalid_argument("require string or buffer for 1st argument");
	}

	boost::scoped_ptr<v8::String::Utf8Value> iv_str;
	char const* iv = nullptr;
	size_t iv_size = 0;
	if (args[1]->IsString())
	{
		// store key in Utf8 string iv_str
		iv_str.reset(new v8::String::Utf8Value(args[1]));
		iv = **iv_str;
		iv_size = iv_str->length();
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[1]))
	{
		iv = buf->data();
		iv_size = buf->size();
	}
	else
	{
		throw std::invalid_argument("require string or buffer for 2nd argument");
	}

	init(key, key_size, iv, iv_size);
	args.GetReturnValue().Set(args.This());
}

void cipher::update_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	boost::scoped_ptr<v8::String::Utf8Value> data_str;
	char const* data = nullptr;
	size_t data_size = 0;
	if (args[0]->IsString())
	{
		// store data in Utf8 string data_str
		data_str.reset(new v8::String::Utf8Value(args[0]));
		data = **data_str;
		data_size = data_str->length();
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		data = buf->data();
		data_size = buf->size();
	}
	else
	{
		throw std::invalid_argument("require string or buffer argument");
	}

	update(data, data_size);
	args.GetReturnValue().Set(args.This());
}

void cipher::final_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char>& result = final();
	return_buffer(args, result);
}

/////////////////////////////////////////////////////////////////////////////
//
// Diffie_Hellman
//
std::vector<std::string> diffie_hellman::groups()
{
	std::vector<std::string> result;
	std::transform(std::begin(modp_groups), std::end(modp_groups), std::back_inserter(result),
		[](modp_group const& group) { return group.name; });
	return result;
}

diffie_hellman::~diffie_hellman()
{
	DH_free(impl_);
}

void diffie_hellman::reset(std::string const& group_name)
{
	modp_group const* group = std::find_if(std::begin(modp_groups), std::end(modp_groups),
		[&group_name](modp_group const& group) { return group.name == group_name; });
	if (group == std::end(modp_groups))
	{
		throw std::invalid_argument("unknown Diffie-Hellman group name " + group_name);
	}
	reset(group->prime, group->prime_size, group->generator, group->generator_size);
}

void diffie_hellman::reset(int prime_length, int generator)
{
	if (impl_) DH_free(impl_);
	impl_ = DH_new();
	if (!DH_generate_parameters_ex(impl_, prime_length, generator, nullptr))
	{
		throw std::runtime_error("Unable to generate prime");
	}
	check_impl();
}

void diffie_hellman::reset(void const* prime, size_t prime_size, int generator)
{
	if (impl_) DH_free(impl_);
	impl_ = DH_new();
	impl_->p = BN_bin2bn(static_cast<unsigned char const*>(prime), static_cast<int>(prime_size), nullptr);
	impl_->g = BN_new();
	BN_set_word(impl_->g, generator);
	check_impl();
}

void diffie_hellman::reset(void const* prime, size_t prime_size, void const* generator, size_t generator_size)
{
	if (impl_) DH_free(impl_);
	impl_ = DH_new();
	impl_->p = BN_bin2bn(static_cast<unsigned char const*>(prime), static_cast<int>(prime_size), nullptr);
	impl_->g = BN_bin2bn(static_cast<unsigned char const*>(generator), static_cast<int>(generator_size), nullptr);
	check_impl();
}

void diffie_hellman::check_impl()
{
	int codes;
	if (!DH_check(impl_, &codes))
	{
		throw std::runtime_error("failed to check Diffie-Hellman parameters");
	}
	if (BN_is_word(impl_->g, DH_GENERATOR_2))
	{
		BN_ULONG const mod = BN_mod_word(impl_->p, 24);
		if (mod == 11 || mod == 23)
		{
			// Unmask DH_NOT_SUITABLE_GENERATOR to accept IETF group parameters
			// Since OpenSSL checks the prime is congruent to 11 when g = 2
			// while the IETF's primes are congruent to 23 when g = 2
			// see http://crypto.stackexchange.com/questions/12961/diffie-hellman-parameter-check-when-g-2-must-p-mod-24-11
			codes &= ~DH_NOT_SUITABLE_GENERATOR;
		}
	}

	if (codes & DH_CHECK_P_NOT_PRIME)
	{
		throw std::runtime_error("failed to check Diffie-Hellman parameters: p is not a prime number");
	}
	if (codes & DH_CHECK_P_NOT_SAFE_PRIME)
	{
		throw std::runtime_error("failed to check Diffie-Hellman parameters: p is not a safe prime number");
	}
	if (codes & DH_UNABLE_TO_CHECK_GENERATOR)
	{
		throw std::runtime_error("failed to check Diffie-Hellman parameters: unable to check generator");
	}
	if (codes & DH_NOT_SUITABLE_GENERATOR)
	{
		throw std::runtime_error("failed to check Diffie-Hellman parameters: g is not a suitable generator");
	}
}

inline std::vector<char> bignum_to_buffer(BIGNUM const* bn)
{
	size_t const size = bn? BN_num_bytes(bn) : 0;

	std::vector<char> result(size);
	if (bn && size)
	{
		BN_bn2bin(bn, reinterpret_cast<unsigned char*>(result.data()));
	}
	return result;
}

void diffie_hellman::generate_keys()
{
	if (!DH_generate_key(impl_))
	{
		throw std::runtime_error("Key pair generation failed");
	}
}

inline std::vector<char> compute_shared_key_impl(DH* dh, BIGNUM const* other_pub_key)
{
	int const result_size = DH_size(dh);
	std::vector<char> result(result_size);
	int const key_size = DH_compute_key(reinterpret_cast<unsigned char*>(result.data()), other_pub_key, dh);
	if (key_size == -1)
	{
		int codes = 0;
		DH_check_pub_key(dh, other_pub_key, &codes);
		if (codes & DH_CHECK_PUBKEY_TOO_SMALL)
		{
			throw std::runtime_error("Public key too small");
		}
		else if (codes & DH_CHECK_PUBKEY_TOO_LARGE)
		{
			throw std::runtime_error("Public key too large");
		}
		else
		{
			throw std::runtime_error("Invalid public key");
		}
	}
	if (key_size < result_size)
	{
		// make the result left padded with zeros to result_size
		// using rotate, since the result has been initialized with zeros
		size_t const pad_count = result_size - key_size;
		std::rotate(result.begin(), result.end() - pad_count, result.end());
	}
	return result;
}

std::vector<char> diffie_hellman::compute_shared_key(diffie_hellman const& other_dh)
{
	return compute_shared_key_impl(impl_, other_dh.impl_->pub_key);
}

std::vector<char> diffie_hellman::compute_shared_key(void const* other_public_key, size_t other_public_key_size)
{
	std::unique_ptr<BIGNUM, decltype(&BN_free)> pub_key(BN_new(), &BN_free);
	BN_bin2bn(reinterpret_cast<unsigned char const*>(other_public_key), static_cast<int>(other_public_key_size), pub_key.get());
	return compute_shared_key_impl(impl_, pub_key.get());
}

std::vector<char> diffie_hellman::prime() const
{
	return bignum_to_buffer(impl_->p);
}

std::vector<char> diffie_hellman::generator() const
{
	return bignum_to_buffer(impl_->g);
}

std::vector<char> diffie_hellman::public_key() const
{
	return bignum_to_buffer(impl_->pub_key);
}

void diffie_hellman::set_public_key(void const* data, size_t size)
{
	impl_->pub_key = BN_bin2bn(reinterpret_cast<unsigned char const*>(data), static_cast<int>(size), nullptr);
}

std::vector<char> diffie_hellman::private_key() const
{
	return bignum_to_buffer(impl_->priv_key);
}

void diffie_hellman::set_private_key(void const* data, size_t size)
{
	impl_->priv_key = BN_bin2bn(reinterpret_cast<unsigned char const*>(data), static_cast<int>(size), nullptr);
}

diffie_hellman::diffie_hellman(v8::FunctionCallbackInfo<v8::Value> const& args)
	: impl_()
{
	v8::Isolate* isolate = args.GetIsolate();

	if (args[0]->IsString())
	{
		std::string const group_name = v8pp::from_v8<std::string>(isolate, args[0]);
		reset(group_name);
	}
	else if (args[0]->IsNumber())
	{
		int const prime_length = v8pp::from_v8<int>(isolate, args[0]);
		int const generator = v8pp::from_v8<int>(isolate, args[1], 2);
		reset(prime_length, generator);
	}
	else if (v8_core::buffer const* prime = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		v8_core::buffer const* generator = v8pp::from_v8<v8_core::buffer*>(isolate, args[1]);
		if (generator)
		{
			reset(prime->data(), prime->size(), generator->data(), generator->size());
		}
		else
		{
			int const generator = v8pp::from_v8<int>(isolate, args[1], 2);
			reset(prime->data(), prime->size(), generator);
		}
	}
	else
	{
		throw std::invalid_argument("require String, Number or Buffer first argument");
	}
}

void diffie_hellman::generate_keys_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	generate_keys();
	return public_key_v8(args);
}

void diffie_hellman::compute_shared_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::vector<char> result;
	if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		result = compute_shared_key(buf->data(), buf->size());
	}
	else if (diffie_hellman const* dh = v8pp::from_v8<diffie_hellman*>(isolate, args[0]))
	{
		result = compute_shared_key(*dh);
	}
	else
	{
		throw std::invalid_argument("require Buffer or DiffieHellman instance for public key argument");
	}

	return_buffer(args, result);
}

void diffie_hellman::prime_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char> result = prime();
	return_buffer(args, result);
}

void diffie_hellman::generator_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char> result = generator();
	return_buffer(args, result);
}

void diffie_hellman::public_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char> result = public_key();
	return_buffer(args, result);
}

void diffie_hellman::set_public_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		set_public_key(buf->data(), buf->size());
	}
	else
	{
		throw std::invalid_argument("require Buffer for public key argument");
	}
}

void diffie_hellman::private_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::vector<char> result = private_key();
	return_buffer(args, result);
}

void diffie_hellman::set_private_key_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		set_private_key(buf->data(), buf->size());
	}
	else
	{
		throw std::invalid_argument("require Buffer for private key argument");
	}
}

/////////////////////////////////////////////////////////////////////////////

typedef unsigned char* (*hash_function)(unsigned char const* data, size_t size, unsigned char* out);

template<hash_function hash, size_t digest_length>
std::string hash_digest_impl(void const* data, size_t size, bool hex)
{
	unsigned char result[digest_length];
	hash(reinterpret_cast<unsigned char const*>(data), size, result);

	return hex? utils::hex_str(result, digest_length) :
		std::string(result, result + digest_length);
}

std::string md5_digest(void const* data, size_t size, bool hex)
{
	return hash_digest_impl<MD5, MD5_DIGEST_LENGTH>(data, size, hex);
}

std::string sha1_digest(void const* data, size_t size, bool hex)
{
	return hash_digest_impl<SHA1, SHA_DIGEST_LENGTH>(data, size, hex);
}

std::vector<char> pkcs5_pbkdf2_hmac(size_t key_size, void const* password, size_t password_size,
	void const* salt, size_t salt_size, unsigned iterations, std::string const& hash_algorithm)
{
	EVP_MD const* md = digestbyname(hash_algorithm);

	std::vector<char> result(key_size);

	if (!PKCS5_PBKDF2_HMAC(static_cast<char const*>(password), static_cast<int>(password_size),
		salt_size? static_cast<unsigned char const*>(salt): nullptr, static_cast<int>(salt_size),
		static_cast<int>(iterations), md,
		static_cast<int>(result.size()), reinterpret_cast<unsigned char*>(result.data())))
	{
		throw std::runtime_error("pkcs5_pbkdf2_hmac failed");
	}
	return result;
}

std::vector<char> random_bytes(size_t size)
{
	std::vector<char> result(size);
	if (!RAND_bytes(reinterpret_cast<unsigned char*>(result.data()), static_cast<int>(result.size())))
	{
		throw std::runtime_error("random_bytes failed");
	}
	return result;
}

std::vector<char> pseudo_random_bytes(size_t size)
{
	std::vector<char> result(size);
	if (!RAND_pseudo_bytes(reinterpret_cast<unsigned char*>(result.data()), static_cast<int>(result.size())))
	{
		throw std::runtime_error("random_bytes failed");
	}
	return result;
}

template<bool pseudo>
static void random_bytes_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	size_t const size = v8pp::from_v8<size_t>(isolate, args[0]);
	std::vector<char> result = pseudo? pseudo_random_bytes(size) : random_bytes(size);
	return_buffer(args, result);
}

static void PBKDF2(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	boost::scoped_ptr<v8::String::Utf8Value> pass_str;
	char const* pass = nullptr;
	size_t pass_size = 0;
	if (args[0]->IsString())
	{
		// store key in Utf8 string pass_str
		pass_str.reset(new v8::String::Utf8Value(args[0]));
		pass = **pass_str;
		pass_size = pass_str->length();
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		pass = buf->data();
		pass_size = buf->size();
	}
	else
	{
		throw std::invalid_argument("required password string or buffer for 1st argument");
	}

	boost::scoped_ptr<v8::String::Utf8Value> salt_str;
	char const* salt = nullptr;
	size_t salt_size = 0;
	if (args[1]->IsString())
	{
		// store salt in Utf8 string pass_str
		salt_str.reset(new v8::String::Utf8Value(args[1]));
		salt = **salt_str;
		salt_size = salt_str->length();
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[1]))
	{
		salt = buf->data();
		salt_size = buf->size();
	}
	else
	{
		throw std::invalid_argument("required salt string or buffer for 2nd argument");
	}

	unsigned const iterations = v8pp::from_v8<unsigned>(isolate, args[2]);
	size_t const keylen = v8pp::from_v8<unsigned>(isolate, args[3]);
	std::string const hash = v8pp::from_v8<std::string>(isolate, args[4], "sha1");

	std::vector<char> result = pkcs5_pbkdf2_hmac(keylen, pass, pass_size, salt, salt_size, iterations, hash);
	return_buffer(args, result);
}

static void encrypt_script(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (args.Length() < 1)
	{
		throw std::invalid_argument("expecting buffer or string as an argument");
	}

	script_container container;

	if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[0]))
	{
		container.encrypt(buf->data(), buf->size());
	}
	else
	{
		v8::String::Utf8Value const str(args[0]);
		container.encrypt(*str, str.length());
	}

	v8_core::buffer* buf = new v8_core::buffer;
	buf->swap(container.storage());
	args.GetReturnValue().Set(v8pp::class_<v8_core::buffer>::import_external(isolate, buf));
}

void setup_bindings(v8pp::module& bindings)
{
	OpenSSL_add_all_algorithms();

	/**
	@module crypto
	**/

	/**
	@class Hash
	Hash generator

	@function Hash(algorithm)
	@param algorithm {String}
	Constructor. Initiaize hash generator with specified algorithm, see #getHashes
	Throws exception on unknown algorithm
	**/
	v8pp::class_<hash_generator> hash_class(bindings.isolate(), v8pp::v8_args_ctor);
	hash_class
		/**
		@function reset([algorithm])
		@param [algorithm] {String}
		@return {Hash}
		Reset hash generator to initial state optionaly changing generator algorithm.
		Throws exception on unknown algorithm.
		**/
		.set("reset", &hash_generator::reset_v8)

		/**
		@function update(data)
		@param data {String|Buffer}
		@return {Hash} `this` to chain calls
		Update generator with new data string or buffer.
		**/
		.set("update", &hash_generator::update_v8)

		/**
		@function digest([hex = true])
		@param [hex = true] {Boolean}
		@return {String}
		Finalize generator. Return hash digest as buffer#Buffer
		or hexadecimal-encoded string if `hex == true`.
	
		Further generator calls are valid only after #Hash.reset
		**/
		.set("digest", &hash_generator::digest_v8)

		/**
		@function algorithm()
		@return {String}
		Current hash algorithm, empty string for unintialized generator.
		**/
		.set("algorithm", &hash_generator::algorithm)
		;

	/**
	@class Hmac
	Hmac generator

	@function Hmac(algorithm, key)
	@param algorithm {String}
	@param key {String|Buffer}
	Constructor. Initiaize hmac generator with specified algorithm and key, see #getHashes
	Throws exception on unknown algorithm.
	**/
	v8pp::class_<hmac_generator> hmac_class(bindings.isolate(), v8pp::v8_args_ctor);
	hmac_class
		/**
		@function reset([algorithm, key])
		@param [algorithm] {String}
		@param [key] {String|Buffer}
		@return {Hmac}
		Reset hmac generator to initial state optionaly changing generator algorithm and key.
		Throws exception on unknown algorithm.
		**/
		.set("reset", &hmac_generator::reset_v8)

		/**
		@function update(data)
		@param data {String|Buffer}
		@return {Hmac} `this` to chain calls
		Update generator with new data string or buffer.
		**/
		.set("update", &hmac_generator::update_v8)

		/**
		@function digest([hex = true])
		@param [hex = true] {Boolean}
		@return {String}
		Finalize generator. Return hmac digest as buffer#Buffer
		or hexadecimal-encoded string if `hex == true`

		Further generator calls are valid only after #Hmac.reset
		**/
		.set("digest", &hmac_generator::digest_v8)

		/**
		@function algorithm()
		@return {String}
		Current hash algorithm, empty string for unintialized generator.
		**/
		.set("algorithm", &hmac_generator::algorithm)
		;

	/**
	@class Cipher
	Cipher class for data encryption and decryption.

	@function Cipher(algorithm, encryption)
	@param algorithm {String}
	@param encryption {Boolean}
	Constructor. Create cipher for specified `algorithm` and `encryption` flag
	(`true` for encryption or `false` for decryption).
	Array of supported cipher algorithms is return by #getCiphers() function.
	**/
	v8pp::class_<cipher> cipher_class(bindings.isolate(), v8pp::v8_args_ctor);
	cipher_class
		/**
		@function algorithm()
		@return {String}
		Cipher algorithm
		**/
		.set("algorithm", &cipher::algorithm)

		/**
		@function encryption()
		@return {Boolean}
		Cipher direction, `true` for encryption and `false` for decryption
		**/
		.set("encryption", &cipher::dir)

		/**
		@function keySize()
		@return {Number}
		Cipher key size
		**/
		.set("keySize", &cipher::key_size)

		/**
		@function ivSize()
		@return {Number}
		Cipher iv size
		**/
		.set("ivSize", &cipher::iv_size)

		/**
		@function blockSize()
		@return {Number}
		Cipher block size
		**/
		.set("blockSize", &cipher::block_size)

		/**
		@function setAutoPadding([auto_padding = true])
		@param [auto_padding=true] {Boolean}
		Set auto padding for the cipher
		**/
		.set("setAutoPadding", &cipher::set_auto_padding_v8)

		/**
		@function isAuthMode()
		@return {Boolean}
		Whether is authentication mode cipher
		**/
		.set("isAuthMode", &cipher::is_auth_mode)

		/**
		@function getAuthTag()
		@return {Buffer}
		Authentication tag availble after #Cipher.final() call
		Returns non-empty buffer only for encryption cipher in authentication mode
		**/
		.set("getAuthTag", &cipher::auth_tag_v8)

		/**
		@function setAuthTag(auth_tag)
		@param auth_tag {Buffer}
		@return {Booolean}
		Set authentication tag after #Cipher.init()
		Allowed for decryption cipher in authentication mode
		**/
		.set("setAuthTag", &cipher::set_auth_tag_v8)

		/**
		@function generateKey(password [, salt [, iterations [, hash]]])
		@param password {String|Buffer}
		@param salt {String|Buffer}, 8 symbols length
		@param iterations {Number} number of iterations
		@param hash {String} see #getHashes()
		@return {Object}
		Create key and iv from password and optional salt. Returns `{ key, iv }` object
		with generated key and initialization vector Buffer object.

		Key generation parameters could be supplied as an object with the same name attributes set:

		```
		var cipher = new crypto.Cipher('aes', crypto.Cipher.encryption);
		var init_info = cipher.generateKey({password: 'secret', salt: '12345678', iterations: 23, hash: 'sha256'});
		cipher.initiv(init_info.key, init_info.iv);
		```
		**/
		.set("generateKey", &cipher::generate_key_v8)

		/**
		@function init(password [, salt [, iterations [, hash]]])
		@param password {String|Buffer}
		@param salt {String|Buffer}, 8 symbols length
		@param iterations {Number} number of iterations
		@param hash {String} see #getHashes()
		@return {Cipher} `this` to chain calls
		Initialize cipher with specified password and optional salt.
		Additionally hash algorithm and iterations count may be provided.
		Initialization parameters could be supplied as an object with the same name attributes set,
		see #Cipher.generateKey()
		**/
		.set("init", &cipher::init_v8)

		/**
		@function initiv(key, iv)
		@param key {String|Buffer}
		@param iv {String|Buffer}
		@return {Cipher} `this` to chain calls
		Initialize cipher with key and initialization vector. Key and initiliaztion vector
		may be generated with #Cipher.generateKey()
		**/
		.set("initiv", &cipher::init_iv_v8)

		/**
		@function update(data)
		@param data {String|Buffer}
		@return {Cipher} `this` to chain calls
		Updates the cipher with `data` to encrypt or decrypt.
		Should be called only after #Cipher.init() or #Cipher.initiv()
		**/
		.set("update", &cipher::update_v8)

		/**
		@function final()
		@return {Buffer}
		Finalize the cipher, return result buffer.
		Further #Cipher.update() calls are allowed only after #Cipher.init() or #Cipher.initiv()
		**/
		.set("final", &cipher::final_v8)
		;

	/**
	@class DiffieHellman
	DiffieHellman key exchanges

	@function DiffieHellman(arg [, generator = 2])
	@param arg {Number|Buffer|String}
	@param [generator=2] {Number|Buffer}
	Create Diffie-Hellman key exchange.

	When `arg` is a `Number`, generate a prime with specified bit length.
	When `arg` is a `Buffer`, use it as a prime.
	Optional `generator` argument could be a `Number` or `Buffer`, otherwise generator value `2` is used.

	When `arg` is a `String`, create a Diffie-Hellman key exchange instance from a predefined group name.
	The supported groups are defined in [RFC 2412](http://www.rfc-editor.org/rfc/rfc2412.txt)
	and [RFC 3526](http://www.rfc-editor.org/rfc/rfc3526.txt):
	  * `modp1`
	  * `modp2`
	  * `modp5`
	  * `modp14`
	  * `modp15`
	  * `modp16`
	  * `modp17`
	  * `modp18`

	See also #DiffieHellman.modGroups()
	**/
	v8pp::class_<diffie_hellman> diffie_hellman_class(bindings.isolate(), v8pp::v8_args_ctor);
	diffie_hellman_class
		/**
		@function modGroups()
		@return {Array}
		Lsit for predefined group names
		**/
		.set("modGroups", &diffie_hellman::groups)

		/**
		@function generateKeys()
		@return {Buffer}
		Generate public and private key pair, return public key
		**/
		.set("generateKeys", &diffie_hellman::generate_keys_v8)

		/**
		@function computeSharedKey(other_public_key)
		@param other_public_key {Buffer|DiffieHellman}
		@return {Buffer}
		Compute she shared key using other public key from `Buffer` or `DiffieHellman` key exchange instance.
		Return computed shared key buffer
		**/
		.set("computeSharedKey", &diffie_hellman::compute_shared_key_v8)
		.set("computeSecret", &diffie_hellman::compute_shared_key_v8)

		/**
		@function getPrime()
		@return {Buffer}
		Prime numer as a binary buffer
		**/
		.set("getPrime", &diffie_hellman::prime_v8)

		/**
		@function getGenerator()
		@return {Buffer}
		Generator numer as a binary buffer
		**/
		.set("getGenerator", &diffie_hellman::generator_v8)

		/**
		@function getPublicKey()
		@return {Buffer}
		Public key as a binary buffer
		**/
		.set("getPublicKey", &diffie_hellman::public_key_v8)

		/**
		@function getPrivateKey()
		@return {Buffer}
		Private key as a binary buffer
		**/
		.set("getPrivateKey", &diffie_hellman::private_key_v8)

		/**
		@function setPublicKey(public_key)
		@param public_key {Buffer}
		Set new public key from a binary buffer
		**/
		.set("setPublicKey", &diffie_hellman::set_public_key_v8)

		/**
		@function setPrivateKey(private_key)
		@param private_key {Buffer}
		Set new private key from a binary buffer
		**/
		.set("setPrivateKey", &diffie_hellman::set_private_key_v8)
		;

	/**
	@module crypto
	**/
	v8pp::module crypto_template(bindings.isolate());
	crypto_template
		.set("Hash", hash_class)
		.set("Hmac", hmac_class)
		.set("Cipher", cipher_class)
		.set("DiffieHellman", diffie_hellman_class)

		/**
		@function getHashes()
		@return {Array}
		Array of supported hash alorithms.
		**/
		.set("getHashes", &hash_generator::algorithms)

		/**
		@function getCiphers()
		@return {Array}
		Array of supported cipher alorithms
		**/
		.set("getCiphers", &cipher::algorithms)

		/*
		@function getPublicKeyAlgorithms()
		@return {Array}
		.set("getPublicKeyAlgorithms", list_public_key_algorithms)
		*/

		/**
		@function PBKDF2(password, salt, iterations, keylen [, hash = 'sha1'])
		@param password {String|Buffer}
		@param salt {String|Buffer}
		@param iterations {Number}
		@param keylen {Number}
		@param [hash=sha1] {String}
		@return {Buffer}
		Password-Based Key Derivation Function 2.
		Derives a key of `keylen` length from a `password` string or buffer
		using a `salt` and `iterations` as specified in
		[RFC 2898](https://www.ietf.org/rfc/rfc2898.txt)
		**/
		.set("PBKDF2", PBKDF2)

		/**
		@function randomBytes(size)
		@param size {Number}
		@return {Buffer}
		Generate cryptographically strong pseudo-random data of specfied `size`
		**/
		.set("randomBytes", random_bytes_v8<false>)

		/**
		@function pseudoRandomBytes(size)
		@param size {Number}
		@return {Buffer}
		Generate non-cryptographically strong pseudo-random data of specfied `size`
		**/
		.set("pseudoRandomBytes", random_bytes_v8<true>)

		.set("encryptScript", encrypt_script)
		;
	bindings.set("crypto", crypto_template);
}

}} // aspect::crypto
