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

#include "jsx/http_logger.hpp"

#if !HAVE(PION_LOGGER_DELEGATE)
#pragma message("WARNING: PION_LOGGER_DELEGATE IS NOT PRESENT!")
#endif
