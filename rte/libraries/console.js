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
@module console - Console
Console output to `stdout` and `stderr`

@property console
Global console instance
**/

var console = (function()
{
	var util = require('util');
	var console = rt.bindings.console;

	console.times = {};

	/**
	@function console.log(fmt [, arg_1 ... arg_n])
	@param fmt {string} - Format string
	@param [args]* - Arguments to substitute to the `fmt` string

	Prints formatted args to `stdout` like `printf` function. If the first
	argument is not a string, util#inspect is used on each argument.
	See util#format for details.
	**/
	console.log = function()
	{
		console.stdout.write(util.format.apply(this, arguments) + '\n');
	}

	/**
	@function console.info(fmt [, arg_1 ... arg_n])
	Same as #console.log
	**/
	console.info = console.log;

	/**
	@function console.error(fmt [, arg_1 ... arg_n])
	Same as #console.log but prints to `stderr`
	**/
	console.error = function()
	{
		console.stderr.write(util.format.apply(this, arguments) + '\n');
	}

	/**
	@function console.warn(fmt [, arg_1 ... arg_n])
	Same as #console.error
	**/
	console.warn = console.error;

	/**
	@function console.dir(object [, options])
	@param object {Object} - Object to print
	@param [options] {Object} - Print options for util#inspect

	Print the result of util#inspect for `object` to `stdout`.
	**/
	console.dir = function(object, inspectOptions)
	{
  		console.stdout.write(util.inspect(object, inspectOptions || {}) + '\n');
	}

	/**
	@function console.time(label)
	@param label

	Start time measuring for specified `label` value.
	**/
	console.time = function(label)
	{
		console.times[label] = Date.now();
	}

	/**
	@function console.timeEnd(label)
	@param label

	Finish timer started by #console.time for the `label` value and print time elapsed since start.
	**/
	console.timeEnd = function(label)
	{
		var time = console.times[label];
		if (!time)
		{
			throw new Error('No such label: ' + label);
		}
		var duration = Date.now() - time;
		this.log('%s: %dms', label, duration);
	}

	/**
	@function console.trace(label)
	@param label

	Print stack trace for a `label` value.
	**/
	console.trace = function()
	{
		var err = new Error;
		err.name = 'Trace';
		err.message = util.format.apply(this, arguments);
		Error.captureStackTrace(err, arguments.callee);
		console.error(err.stack);
	}

	function AssertionError(message, stackStartFunction)
	{
  		this.name = 'AssertionError';
  		this.message = message;
  		Error.captureStackTrace(this, stackStartFunction);
  	}
  	util.inherits(AssertionError, Error);

  	/**
	@function console.assert(expression [, message])
	@param expression
	@param [message] {String}

	Check an `expression` and throw an `AssertionError` with optional `message` if the expression evaluates as `false`.
  	**/
	console.assert = function(expression, message)
	{
		if (!!!expression)
		{
			throw new AssertionError(message, console.assert);
		}
	}

	return console;

})();

exports.$ = console;
