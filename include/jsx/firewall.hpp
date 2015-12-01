//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_FIREWALL_HPP_INCLUDED
#define JSX_FIREWALL_HPP_INCLUDED

#include "jsx/v8_core.hpp"

#include <boost/filesystem/path.hpp>

namespace aspect { namespace firewall {

void setup_bindings(v8pp::module& target);

void add_rule(std::wstring const& name, boost::filesystem::path const& program);
void remove_rule(boost::filesystem::path const& program);

}} // namespace aspect::filrewall

#endif // JSX_FIREWALL_HPP_INCLUDED
