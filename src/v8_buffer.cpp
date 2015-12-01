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
#include "jsx/v8_buffer.hpp"

#include "jsx/aes.hpp"
#include "jsx/crypto.hpp"
#include "jsx/runtime.hpp"

#pragma warning(push)
#pragma warning(disable: 4244 4702 4706)
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#pragma warning(pop)

namespace aspect { namespace v8_core {

void buffer::setup_bindings(v8pp::module& target)
{
	/**
	@module buffer Buffer

	@class Buffer
	Buffer class
	Represent a byte buffer.

	@function Buffer([arg] [, encoding])
	@param [arg] {Buffer|String|Number}
	@param [encoding] {String}
	Create a buffer.

	Optional paramter `arg` may one of following types:
	  * `Buffer` or `String` - copy data from source buffer or string
	  * `Number` - create a buffer of specified size filled with zeros.

	If the `arg` is a String or Buffer, optinal `encoding` parameter could be used
	to specify the string encoding, one of:
	  * `hex`
	  * `base64`

	See also #Buffer.fromString
	**/
	v8pp::class_<buffer> buffer_class(target.isolate(), v8pp::v8_args_ctor);
	buffer_class
		/**
		@property length {Number}
		Number of bytes in the buffer, read-only.
		**/
		.set("length", v8pp::property(&buffer::size))

		/**
		@property size {Number}
		Number of bytes in the buffer.
		Resize buffer on the property change.
		**/
		.set("size", v8pp::property(&buffer::size, &buffer::resize))

		/**
		@function clear()
		Make the buffer empty. Similar to `buffer.size = 0`.
		**/
		.set("clear", &buffer::clear)

		/**
		@function append(buf)
		@param buf {Buffer}
		@return {Buffer}
		Make a copy of the buffer with data appended from `buf`.
		**/
		.set("append", &buffer::append)

		/**
		@function string()
		@return {String}
		Return buffer data as a string.
		**/
		.set("string", &buffer::string)

		/**
		@function substring(start [, length])
		@param start {Number}
		@param [length] {Number} default value is `this.length - start`
		@return {String}
		Return buffer data as a string starting from `start` with maximal `length`.
		**/
		.set("substring", &buffer::substring)

		/**
		@function slice(start [, length])
		@param start {Number}
		@param [length] {Number} default value is `this.length - start`
		@return {Buffer}
		Return buffer data as a new buffer starting from `start` with maximal `length`.
		**/
		.set("slice", &buffer::slice)

		/**
		@function copy(offset, src [, src_offset = 0, src_length = src.length])
		@param offset {Number}
		@param src {String|Buffer}
		@param [src_offset] {Number} default value is `0`
		@param [src_length] {Number} default value is `src.length - src_offset`
		@return {Buffer} this to chain calls
		Copy data from `src` string or buffer to the buffer starting from `offset`.
		Optinal `src_offset` and `src_length` values specify data offset and length in
		the `src`.
		**/
		.set("copy", &buffer::copy)

		/**
		@function hash()
		@param type {String}
		@return {String}
		Return hex digest string for the buffer data.
		Hash type is one of values return by crypto#getHashes(), for example:
		  * `sha1`
		  * `sha256`
		  * `sha512`

		See also crypto#hashDigest()
		**/
		.set("hash", &buffer::hash)

		/**
		@function hmac(type, key)
		@param type {String}
		@param key {String|Buffer}
		@return {Buffer} this
		Return HMAC `type` hex digest from the buffer data and specified `key`.
		HMAC type is one of values return by crypto#getHashes(), for example:
		  * `sha1`
		  * `sha256`
		  * `sha512`

		See also crypto#hmacDigest()
		**/
		.set("hmac", &buffer::hmac)

		/**
		@function toString([encoding])
		@param [encoding] {String}
		@return {String}
		Return buffer data as a string.
		Optional `encoding`  parameter is one of:
		  * `hex`
		  * `base64`
		**/
		.set("toString", &buffer::to_string)

		/**
		@function fromString(string, [encoding])
		@param string {String}
		@param [encoding] {String}
		@return {Buffer}
		Create a Buffer instance from a string.
		Optional `encoding`  parameter is one of:
		  * `hex`
		  * `base64`
		**/
		.set("fromString", &buffer::from_string)

		/**
		@function dump([width = 24])
		@param [width=24] {Number}
		@return {String}
		Return buffer data as a hex dump multi line string with specified line width.
		**/
		.set("dump", &buffer::dump)

		/**
		@function compare(buf)
		@param buf {Buffer}
		@return {Boolean}
		Compare for equality buffer data with another `buf` data.
		**/
		.set("compare", &buffer::compare)

		/**
		@function compress()
		@return {Buffer} this
		Compress buffer data in-place.
		**/
		.set("compress", &buffer::compress)

		/**
		@function decompress()
		@return {Buffer} this
		Decompress buffer data in-place.
		**/
		.set("decompress", &buffer::decompress)

		/**
		@function encrypt(key [, iv])
		@param key {String}
		@param [iv] {String}
		@return {Buffer} this
		In-place Rijndael encryption of buffer data with 128-bit `key` string and
		optional `iv` initialization vector string.
		**/
		.set("encrypt", &buffer::encrypt)

		/**
		@function decrypt(key [, iv])
		@param key {String}
		@param [iv] {String}
		@return {Buffer} this
		In-place Rijndael decryption of buffer data with 128-bit `key` string and
		optional `iv` initialization vector string.
		**/
		.set("decrypt", &buffer::decrypt)

		/**
		@function getUint32(pos)
		@param pos {Number}
		Return buffer data at position `pos` as an unsigned 32-bit integer.
		There are also getUint64, getInt64, getInt32, getUint16, getInt16, getUint8, getInt8,
		getFloat32, getFloat64 similar functions.
		**/
		.set("getUint8", &buffer::get<uint8_t>)
		.set("getInt8", &buffer::get<int8_t>)
		.set("getUint16", &buffer::get<uint16_t>)
		.set("getInt16", &buffer::get<int16_t>)
		.set("getUint32", &buffer::get<uint32_t>)
		.set("getInt32", &buffer::get<int32_t>)
		.set("getUint64", &buffer::get<uint64_t>)
		.set("getInt64", &buffer::get<int64_t>)
		.set("getFloat32", &buffer::get<float>)
		.set("getFloat64", &buffer::get<double>)

		/**
		@function setUint32(pos, v)
		@param pos {Number}
		@param v {Number}
		Set buffer data at position `pos` as an unsigned 32-bit integer `v`.
		There are also setUint64, setInt64, setInt32, setUint16, setInt16, setUint8, setInt8,
		setFloat32, setFloat64 similar functions.
		**/
		.set("setUint8", &buffer::set<uint8_t>)
		.set("setInt8", &buffer::set<int8_t>)
		.set("setUint16", &buffer::set<uint16_t>)
		.set("setInt16", &buffer::set<int16_t>)
		.set("setUint32", &buffer::set<uint32_t>)
		.set("setInt32", &buffer::set<int32_t>)
		.set("setUint64", &buffer::set<uint64_t>)
		.set("setInt64", &buffer::set<int64_t>)
		.set("setFloat32", &buffer::set<float>)
		.set("setFloat64", &buffer::set<double>)

		/**
		@function appendUint32(v)
		@param v {Number}
		Append to the buffer an unsigned 32-bit integer `v`.
		There are also appendUint64, appendInt64, appendInt32, appendUint16, appendInt16, appendUint8, appendInt8
		appendFloat32, appendFloat64 similar functions.
		**/
		.set("appendUint8", &buffer::push<uint8_t>)
		.set("appendInt8", &buffer::push<int8_t>)
		.set("appendUint16", &buffer::push<uint16_t>)
		.set("appendInt16", &buffer::push<int16_t>)
		.set("appendUint32", &buffer::push<uint32_t>)
		.set("appendInt32", &buffer::push<int32_t>)
		.set("appendUint64", &buffer::push<uint64_t>)
		.set("appendInt64", &buffer::push<int64_t>)
		.set("appendFloat32", &buffer::push<float>)
		.set("appendFloat64", &buffer::push<double>)
	;

	target.set("Buffer", buffer_class);
}

// ---
// buffer
buffer::buffer(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	std::string const encoding = v8pp::from_v8<std::string>(isolate, args[1], "");

	v8::Local<v8::Value> arg = args[0];
	if (buffer const* buf = v8pp::from_v8<buffer*>(isolate, arg) )
	{
		decode(encoding, buf->data(), buf->size());
	}
	else if (arg->IsString())
	{
		v8::String::Utf8Value const str(arg);
		decode(encoding, *str, str.length());
	}
	else if (arg->IsNumber())
	{
		resize(arg->ToUint32()->Value());
	}

	if (args[1]->IsNumber())
	{
		v8::ExternalArrayType eat = v8::kExternalUnsignedByteArray;
		uint32_t const flags = v8pp::from_v8<uint32_t>(isolate, args[1], 0);
		if ( flags >= v8::kExternalByteArray && flags <= v8::kExternalFloatArray )
		{
			eat = static_cast<v8::ExternalArrayType>(flags);
		}
		args.This()->SetIndexedPropertiesToExternalArrayData(data(), eat, static_cast<int>(size()));
	}
}

void buffer::decode(std::string const& encoding, char const* data, size_t size)
{
	size_t const prev_capacity = impl_.capacity();

	if (encoding.empty())
	{
		impl_.assign(data, data + size);
	}
	else if (encoding == "hex")
	{
		utils::from_hex_str(impl_, data, size);

	}
	else if (encoding == "base64")
	{
		utils::decode_base64(impl_, data, size);
	}
	else
	{
		throw std::invalid_argument("unknown encoding " + encoding);
	}

	intptr_t const mem_diff = impl_.capacity() - prev_capacity;
	v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(mem_diff);
}

void buffer::resize(size_t new_size)
{
	size_t const prev_capacity = impl_.capacity();
	impl_.resize(new_size);

	intptr_t const mem_diff = impl_.capacity() - prev_capacity;
	v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(mem_diff);
}

void buffer::replace(char const* data, size_t size)
{
	size_t const prev_capacity = impl_.capacity();

	if (data)
	{
		impl_.assign(data, data + size);
	}
	else
	{
		impl_.resize(size, 0);
	}

	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	intptr_t const mem_diff = impl_.capacity() - prev_capacity;
	isolate->AdjustAmountOfExternalAllocatedMemory(mem_diff);

	v8::Local<v8::Value> obj = v8pp::to_v8(isolate, this);
	if( !obj.IsEmpty() && obj->IsObject() )
	{
		obj->ToObject()->SetIndexedPropertiesToExternalArrayData(this->data(),
			v8::kExternalUnsignedByteArray, (int)this->size());
	}
}

void buffer::swap(std::vector<char>& buf)
{
	size_t const prev_capacity = impl_.capacity();

	impl_.swap(buf);

	intptr_t const mem_diff = impl_.capacity() - prev_capacity;
	v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(mem_diff);
}

v8::Handle<v8::Value> buffer::append(buffer const& srcB)
{
	buffer const& srcA = *this;
	size_t const target_size = srcA.size() + srcB.size();

	std::unique_ptr<buffer> target(new buffer(nullptr, target_size));

	memcpy(target->data(), srcA.data(), srcA.size());
	memcpy(target->data() + srcA.size(), srcB.data(), srcB.size());

	return v8pp::class_<buffer>::import_external(v8::Isolate::GetCurrent(), target.release());
}

std::string buffer::string() const
{
	std::string str;
	if (!impl_.empty())
	{
		size_t const len = impl_.size() - (impl_.back() == 0);
		str.assign(impl_.begin(), impl_.begin() + len);
	}
	return str;
}

std::string buffer::substring(v8::FunctionCallbackInfo<v8::Value> const& args) const
{
	if (impl_.empty())
	{
		throw std::runtime_error("buffer::substring() - buffer is empty");
	}

	v8::Isolate* isolate = args.GetIsolate();

	size_t const start = v8pp::from_v8<size_t>(isolate, args[0], std::string::npos);
	size_t const len = std::min(v8pp::from_v8<size_t>(isolate, args[1], std::string::npos), impl_.size() - start);

	if (start == std::string::npos)
	{
		throw std::invalid_argument("buffer::substring() requires at least one integer start argument");
	}
	if (start > impl_.size())
	{
		throw std::runtime_error("buffer::substring() - start is past buffer size");
	}
	if (len == 0)
	{
		throw std::runtime_error("buffer::substring() - length is zero");
	}
	return std::string(&impl_[start], len);
}

v8::Handle<v8::Value> buffer::slice(v8::FunctionCallbackInfo<v8::Value> const& args) const
{
	if (impl_.empty())
	{
		throw std::runtime_error("buffer::slice() - buffer is empty");
	}

	if ( !args.Length() || !args[0]->IsNumber() )
	{
		throw std::invalid_argument("buffer::slice() requires at least one integer argument");
	}

	v8::Isolate* isolate = args.GetIsolate();

	size_t const start = v8pp::from_v8<size_t>(isolate, args[0], std::string::npos);
	size_t const len = std::min(v8pp::from_v8<size_t>(isolate, args[1], std::string::npos), impl_.size() - start);

	if (start == std::string::npos)
	{
		throw std::invalid_argument("buffer::slice() requires at least one integer start argument");
	}
	if (start > impl_.size())
	{
		throw std::runtime_error("buffer::sclice() - start is past buffer size");
	}
	if (len == 0)
	{
		throw std::runtime_error("buffer::sclice() - length is zero");
	}
	return v8pp::class_<buffer>::import_external(isolate, new buffer(data() + start, len));
}

buffer& buffer::copy(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if ( empty() )
	{
		throw std::runtime_error("buffer::copy() - buffer is empty");
	}

	if ( args.Length() < 2 )
	{
		throw std::invalid_argument("buffer::copy(dst_offset,src,src_offset,src_size) requires at least two arguments dst_offset and src");
	}

	v8::Isolate* isolate = args.GetIsolate();

	std::string src_string;

	char const* data = nullptr;
	size_t size = 0;

	size_t const dst_offset = v8pp::from_v8<size_t>(isolate, args[0]);

	if (args[1]->IsString())
	{
		src_string = v8pp::from_v8<std::string>(isolate, args[1]);
		data = src_string.data();
		size = src_string.size();
	}
	else if (buffer const* src_buffer = v8pp::from_v8<buffer*>(isolate, args[1]))
	{
		data = src_buffer->data();
		size = src_buffer->size();
	}
	else
	{
		throw std::invalid_argument("buffer::copy() - (second argument) buffer source 'src' is of an unknown type");
	}

	size_t const src_offset = v8pp::from_v8<size_t>(isolate, args[2], 0);
	size_t const src_length = v8pp::from_v8<size_t>(isolate, args[3], size);

	if (src_offset + src_length > size)
	{
		throw std::runtime_error("buffer::copy() - reading past the end of the source buffer"); //supplied src_length is longer then src_length of the buffer");
	}
	if (dst_offset + src_length > this->size())
	{
		throw std::runtime_error("buffer::copy() - writing past end of destination buffer"); //supplied src_length is longer then src_length of the buffer");
	}

	memcpy(this->data() + dst_offset, data, src_length);

	return *this;
}

std::string buffer::to_string(v8::FunctionCallbackInfo<v8::Value> const& args) const
{
	std::string const encoding = v8pp::from_v8<std::string>(args.GetIsolate(), args[0], "");
	if (encoding.empty())
	{
		return std::string(impl_.begin(), impl_.end());
	}
	else if (encoding == "hex")
	{
		return utils::hex_str(data(), size());
	}
	else if (encoding == "base64")
	{
		return utils::encode_base64(data(), size());
	}
	else
	{
		throw std::invalid_argument("unknown encoding " + encoding);
	}
}

v8::Handle<v8::Value> buffer::from_string(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return v8pp::class_<buffer>::import_external(args.GetIsolate(), new buffer(args));
}

std::string buffer::hash(std::string const& type) const
{
	return crypto::hash_generator(type).update(data(), size()).digest(true);
}

std::string buffer::hmac(v8::FunctionCallbackInfo<v8::Value> const& args) const
{
	v8::Isolate* isolate = args.GetIsolate();

	std::string const type = v8pp::from_v8<std::string>(isolate, args[0]);
	if (buffer const* buf = v8pp::from_v8<buffer*>(isolate, args[1]))
	{
		return crypto::hmac_generator(type, buf->data(), buf->size()).update(data(), size()).digest(true);
	}
	else if (args[1]->IsString())
	{
		v8::String::Utf8Value const str(args[1]);
		return crypto::hmac_generator(type, *str, str.length()).update(data(), size()).digest(true);
	}
	else
	{
		throw std::invalid_argument("require String or Buffer for 2nd argument");
	}
}

buffer& buffer::compress()
{
	typedef boost::iostreams::array_source input_device;
	typedef boost::iostreams::back_insert_device<std::vector<char>> output_device;

	std::vector<char> result;

	boost::iostreams::stream<input_device> stream_in(data(), size());
	boost::iostreams::stream<output_device> stream_out(result);

	boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
	in.push(boost::iostreams::zlib_compressor());
	in.push(stream_in);

	boost::iostreams::copy(in, stream_out);
	swap(result);

	return *this;
}

buffer& buffer::decompress()
{
	typedef boost::iostreams::array_source input_device;
	typedef boost::iostreams::back_insert_device<std::vector<char>> output_device;

	std::vector<char> result;

	boost::iostreams::stream<input_device> stream_in(data(), size());
	boost::iostreams::stream<output_device> stream_out(result);

	boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
	in.push(boost::iostreams::zlib_decompressor());
	in.push(stream_in);

	boost::iostreams::copy(in, stream_out);
	swap(result);

	return *this;
}

void buffer::dump(v8::FunctionCallbackInfo<v8::Value> const& args) const
{
	v8::Isolate* isolate = args.GetIsolate();
	runtime const& rt = runtime::instance(isolate);

	size_t const width = v8pp::from_v8<size_t>(isolate, args[0], 24);

	if ( empty() )
	{
		rt.warning("NO DATA");
	}

	std::string str;
	std::string chars;

	char const* data = this->data();
	for (size_t i = 0, length = this->size(); i != length; ++i)
	{
		if (i > 0 && (i % width) == 0)
		{
			rt.trace("%s  %s\n", str.c_str(), chars.c_str());
			str.clear(); chars.clear();
		}

		int const ch = data[i];
		char buf[4];
		snprintf(buf, sizeof(buf), "%02x ", ch);
		str += buf;
		chars += (isprint(ch)? data[i] : '.');
	}

	if ( !str.empty() )
	{
		rt.trace("%s  %s\n",str.c_str(),chars.c_str());
	}
}

buffer& buffer::encrypt(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::string const key = v8pp::from_v8<std::string>(args.GetIsolate(), args[0], "");
	std::string const vec = v8pp::from_v8<std::string>(args.GetIsolate(), args[1], "");

	if ( empty() )
	{
		throw std::runtime_error("encrypt() - buffer is empty");
	}
	if ( key.empty() )
	{
		throw std::runtime_error("encrypt() require key string argument");
	}

	std::vector<char> result;
	AES::encrypt_128_ecb(result, data(), size(), key, vec);
	swap(result);

	return *this;
}

buffer& buffer::decrypt(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::string const key = v8pp::from_v8<std::string>(args.GetIsolate(), args[0], "");
	std::string const vec = v8pp::from_v8<std::string>(args.GetIsolate(), args[1], "");

	if ( empty() )
	{
		throw std::runtime_error("decrypt() - buffer is empty");
	}
	if ( key.empty() )
	{
		throw std::runtime_error("decrypt() require key string argument");
	}

	std::vector<char> result;
	AES::decrypt_128_ecb(result, data(), size(), key, vec);
	swap(result);

	return *this;
}

}} // aspect::v8_core
