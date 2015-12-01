//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_SCRIPT_STORE_HPP_INCLUDED
#define JSX_SCRIPT_STORE_HPP_INCLUDED

#include <vector>

namespace aspect {

/// Script text container
class script_container
{
public:

	/// Create an empty script container
	script_container()
		: src_data_(nullptr)
		, src_size_(0)
	{
	}

	/// Encrypt data[size]
	void encrypt(char const* data, size_t size);

	/// Decrypt data[size]
	void decrypt(char const* data, size_t size);

	/// Script data
	char const* data() const { return data_.empty()? src_data_ : &data_[0]; };

	/// Script data size
	size_t size() const { return data_.empty()? src_size_ : data_.size(); }

	std::vector<char>& storage() { return data_; }
private:
	char const* src_data_;
	size_t src_size_;
	std::vector<char> data_;
};

} // ::aspect

#endif // JSX_SCRIPT_STORE_HPP_INCLUDED
