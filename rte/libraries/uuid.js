//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

/**
@module uuid UUID
**/
var uuid = (function()
{
	var uuid = rt.bindings.Uuid;

	/**
	@function isValid(str)
	@param str {String}
	@return {Boolean} wether `str` is a valid UUID string
	**/
	uuid.prototype.isValid = function(str)
	{
		return typeof(str) === 'string' && str.length === 36;
	}

	return uuid;
	
})();

exports.$ = uuid;