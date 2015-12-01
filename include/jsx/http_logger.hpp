//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_HTTP_LOGGER_HPP_INCLUDED
#define JSX_HTTP_LOGGER_HPP_INCLUDED

#if HAVE(PION_LOGGER_DELEGATE)

#include "jsx/http.hpp"
#include "jsx/log.hpp"

namespace aspect { namespace http {

class pion_logger_binding : public pion::logger_delegate
{
public:
	pion_logger_binding(logger_delegate_ptr delegate)
		: delegate_(delegate)
	{
	}

private:
	virtual void output(pion::log_priority level, const char *msg)
	{
		delegate_->output(level | log::FLAG_LOCAL, "PION", "HTTP - %s", msg);
	}

	logger_delegate_ptr delegate_;
};

}} // aspect::http

#endif // HAVE(PION_LOGGER_DELEGATE)

#endif // JSX_HTTP_LOGGER_HPP_INCLUDED
