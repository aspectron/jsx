//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_HTTP_HPP_INCLUDED
#define JSX_HTTP_HPP_INCLUDED

#include "jsx/core.hpp"
#include "jsx/v8_core.hpp"

#pragma warning(push, 3)
#include <pion/hash_map.hpp>
#include <pion/logger.hpp>

#include <pion/http/request.hpp>
#include <pion/http/response.hpp>
#include <pion/http/response_writer.hpp>

#include <pion/http/plugin_service.hpp>
#include <pion/http/plugin_server.hpp>

#pragma warning(pop)

namespace aspect { namespace http {

void setup_bindings(v8pp::module& target);

void set_log_level(int level);

v8::Handle<v8::Object> dict_to_object(v8::Isolate* isolate, pion::ihash_multimap const& dict);
pion::ihash_multimap object_to_dict(v8::Isolate* isolate, v8::Handle<v8::Object> object);

}} // ::aspect::http

#endif // JSX_HTTP_HPP_INCLUDED
