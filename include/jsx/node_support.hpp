//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_NODE_SUPPORT_HPP_INCLUDED
#define JSX_NODE_SUPPORT_HPP_INCLUDED

#include <v8.h>

namespace aspect { namespace node_support {

/// Initialize Node.js support
void init(v8::Isolate* isolate, bool enable);

}} // aspect::node_support

#endif // JSX_NODE_SUPPORT_HPP_INCLUDED
