//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

JSON.to_string = JSON.stringify;

JSON.to_string_with_functions = function(obj, space)
{
    var serialize_with_functions = function(key, obj) {
        return typeof(obj) == 'function'? obj.toString() : obj;
    };

    return JSON.stringify(obj, serialize_with_functions, space);
}

exports.$ = JSON;