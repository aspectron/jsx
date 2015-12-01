//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_V8_BUFFER_HPP_INCLUDED
#define JSX_V8_BUFFER_HPP_INCLUDED

#include "jsx/v8_core.hpp"
#include "jsx/types.hpp"

namespace aspect { namespace v8_core {

class CORE_API buffer
{
public:
	static void setup_bindings(v8pp::module& target);

	explicit buffer(v8::FunctionCallbackInfo<v8::Value> const& args);

	explicit buffer(char const* data = nullptr, size_t size = 0)
	{
		replace(data, size);
	}

	explicit buffer(std::vector<char> const& buf)
	{
		replace(buf);
	}

	buffer(buffer const& src)
	{
		replace(src.impl_);
	}

	buffer& operator=(buffer const& src)
	{
		if (&src != this)
		{
			replace(src.impl_);
		}
		return *this;
	}

	~buffer() { clear(); }

	char* data() { return impl_.empty()? nullptr : &impl_[0]; }
	char const* data() const { return impl_.empty()? nullptr : &impl_[0]; }

	size_t size() const { return impl_.size(); }
	bool empty() const { return impl_.empty(); }

	void clear() { resize(0); }
	void resize(size_t new_size);

	void replace(char const* data, size_t size);
	void replace(std::vector<char> const& buf)
	{
		replace(buf.empty()? nullptr : &buf[0], buf.size());
	}

	// swap the buffer content with vector<char> data (as move emulation in C++03)
	void swap(std::vector<char>& buf);
	void swap(std::vector<unsigned char>& buf)
	{
		swap(reinterpret_cast<std::vector<char>&>(buf));
	}

	void dump(v8::FunctionCallbackInfo<v8::Value> const& args) const;

	buffer& copy(v8::FunctionCallbackInfo<v8::Value> const& args);
	v8::Handle<v8::Value> append(buffer const& src);
	v8::Handle<v8::Value> slice(v8::FunctionCallbackInfo<v8::Value> const& args) const;

	std::string string() const;
	std::string substring(v8::FunctionCallbackInfo<v8::Value> const& args) const;

	bool compare(buffer const& src) const
	{
		return impl_ == src.impl_;
	}

	std::string to_string(v8::FunctionCallbackInfo<v8::Value> const& args) const;
	static v8::Handle<v8::Value> from_string(v8::FunctionCallbackInfo<v8::Value> const& args);

	std::string hash(std::string const& type) const;
	std::string hmac(v8::FunctionCallbackInfo<v8::Value> const& args) const;

	buffer& compress();
	buffer& decompress();

	buffer& encrypt(v8::FunctionCallbackInfo<v8::Value> const& args);
	buffer& decrypt(v8::FunctionCallbackInfo<v8::Value> const& args);

	template<typename T>
	typename boost::enable_if<boost::is_arithmetic<T>, T>::type
	get(size_t byte_pos) const
	{
		if (byte_pos + sizeof(T) > size())
		{
			throw std::runtime_error("out of bounds");
		}
		T value;
		memcpy(&value, data() + byte_pos, sizeof(value));
		return value;
	}

	template<typename T>
	typename boost::enable_if<boost::is_arithmetic<T>, buffer&>::type
	set(size_t byte_pos, T value)
	{
		if (byte_pos + sizeof(T) > size())
		{
			throw std::runtime_error("out of bounds");
		}
		memcpy(data() + byte_pos, &value, sizeof(value));
		return *this;
	}

	template<typename T>
	typename boost::enable_if<boost::is_arithmetic<T>, buffer&>::type
	push(T value)
	{
		resize(size() + sizeof(value));
		return set(size() - sizeof(value), value);
	}

private:
	void decode(std::string const& encoding, char const* data, size_t size);
	std::vector<char> impl_;
};

}} // aspect::v8_core

#endif // JSX_V8_BUFFER_HPP_INCLUDED
