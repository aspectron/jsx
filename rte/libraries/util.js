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
@module util - Utilities
Utility functions
**/

Array.prototype.push_string_charcodes = function(str)
{
    for (var index = 0; index < str.length; index++) {
        this.push(str.charCodeAt(index));
    }
}

function objectToString(obj) { return Object.prototype.toString.call(obj); }
function hasOwnProperty(obj, prop) { return Object.prototype.hasOwnProperty.call(obj, prop); }

/**
@function isArray(value)
@param value
@return {Boolean} - whether `value` is a JavaScript `Array` instance.
**/
exports.isArray = function(value)
{
    return Array.isArray(value) ||
        (typeof value === 'object' && objectToString(value) === '[object Array]');
};

/**
@function isRegExp(value)
@param value
@return {Boolean} - whether `value` is a JavaScript `RegExp` instance.
**/
exports.isRegExp = function(value)
{
    return typeof value === 'object' && objectToString(value) === '[object RegExp]';
}

/**
@function isDate(value)
@param value
@return {Boolean} - whether `value` is a JavaScript `Date` instance.
**/
exports.isDate = function(value)
{
    return typeof value === 'object' && objectToString(value) === '[object Date]';
}

/**
@function isError(value)
@param value
@return {Boolean} - whether `value` is a JavaScript `Error` instance.
**/
exports.isError = function(value)
{
    return typeof value === 'object' && objectToString(value) === '[object Error]';
}


/**
@function extend(target, object [, object..])
@param target {Object}
@param object* {Object}
@return {Object}
Merge the contents of two or more objects together into the first object.
See [`jQuery.extend()`](http://api.jquery.com/jquery.extend/)
**/
exports.extend = function() 
{
    // copy reference to target object
    var target = arguments[0] || {}, i = 1, length = arguments.length, deep = false, options, name, src, copy;

    // Handle a deep copy situation
    if ( typeof target === "boolean" ) {
        deep = target;
        target = arguments[1] || {};
        // skip the boolean and the target
        i = 2;
    }

    // Handle case when target is a string or something (possible in deep copy)
    if ( typeof(target) !== "object" && typeof(target) != 'function' ) 
    {
        target = {};
    }

    // extend current context (this) if only one argument is passed
    if ( length === i ) 
    {
        target = this;
        --i;
    }

    for ( ; i < length; i++ ) 
    {
        // Only deal with non-null/undefined values
        if ( (options = arguments[ i ]) != null ) 
        {
            // Extend the base object
            for ( name in options ) 
            {
                src = target[ name ];
                copy = options[ name ];

                // Prevent never-ending loop
                if ( target === copy ) 
                {
                    continue;
                }

                // Recurse if we're merging object literal values or arrays
                if ( deep && copy && ( typeof(copy) == 'object' /*|| isArray(copy)*/ ) ) 
                {
                    //var clone = src && ( typeof(src) == 'object' || isArray(src) ) ? src
                    //  : typeof(copy) == 'array' ? [] : {};
                    
                    var clone = src && (typeof(src) == 'object') ? src : isArray(src) ? [] : {};

                    // Never move original objects, clone them
                    target[ name ] = extend( deep, clone, copy );

                    // Don't bring in undefined values
                } 
                else if ( copy !== undefined ) 
                {
                    target[ name ] = copy;
                }
            }
        }
    }

    // Return the modified object
    return target;
}

/**
@function inherits(ctor, superCtor)
@param ctor {Object}
@param superCtor {Object}
@return {Object}
Inherit the prototype methods from one constructor into another. The prototype
of `ctor` will be set to a new object created from `superCtor` and 
`superCtor` will be accessible through the `ctor.super_` property.
**/
exports.inherits = function(ctor, superCtor) {
  ctor.super_ = superCtor;
  ctor.prototype = Object.create(superCtor.prototype, {
    constructor: {
      value: ctor,
      enumerable: false,
      writable: true,
      configurable: true
    }
  });
};

/**
@function each(object, callback [, arg_1 .. arg_n])
@param object {Object}
@param callback {Function}
@param [args]*
Apply `callback` function to each element in object passing optional `args`.
**/
exports.each = function( object, callback, args ) 
{
    if(!object)
        return object;
        
    var name, i = 0,
        length = object.length,
        isObj = length === undefined || typeof(object) == 'function';

    if ( args ) 
    {
        if ( isObj ) 
        {
            for ( name in object ) 
            {
                if ( callback.apply( object[ name ], args ) === false ) 
                {
                    break;
                }
            }
        } 
        else 
        {
            for ( ; i < length; ) 
            {
                if ( callback.apply( object[ i++ ], args ) === false ) 
                {
                    break;
                }
            }
        }

    // A special, fast, case for the most common use of each
    } 
    else 
    {
        if ( isObj ) 
        {
            for ( name in object ) 
            {
                if ( callback.call( object[ name ], name, object[ name ] ) === false ) 
                {
                    break;
                }
            }
        } 
        else 
        {
            for ( var value = object[0];
                i < length && callback.call( value, i, value ) !== false; value = object[++i] ) {}
        }
    }

    return object;
}

/**
@function inspect(object [, options])
@param object {Object}
@param [options] {Object}
@return {String}
Convert `object` to a string. An optional `options` object may be specified
with the following properties set:
    
  * `showHidden`   Boolean flag, when `true`, all object properties will be enumerated. Default value is `false`.
  * `depth`        Maximum recursion depth for object properties enumeration. Default value is `2`.
  * `colors`       Boolean flag, when `true` output will be colored.

Here is an example how to inspect all `util` object properties:

    var util = require('util');

    console.log(util.inspect(util, { showHidden: true, depth: Infinity, colors: true }));
**/
exports.inspect = function(obj, options)
{
    var ctx = options || {};

    ctx.visited = [];
    ctx.depth = ctx.depth || 2;
    ctx.stylize = ctx.colors? coloredString : simpleString;

    return formatValue(ctx, obj, ctx.depth);
}

// http://en.wikipedia.org/wiki/ANSI_escape_code#graphics
exports.inspect.colors = {
    'bold' : [1, 22],
    'italic' : [3, 23],
    'underline' : [4, 24],
    'inverse' : [7, 27],
    'white' : [37, 39],
    'grey' : [90, 39],
    'black' : [30, 39],
    'blue' : [34, 39],
    'cyan' : [36, 39],
    'green' : [32, 39],
    'magenta' : [35, 39],
    'red' : [31, 39],
    'yellow' : [33, 39]
};

// Don't use 'blue' not visible on cmd.exe
exports.inspect.styles = {
    'special': 'cyan',
    'number': 'yellow',
    'boolean': 'yellow',
    'undefined': 'grey',
    'null': 'bold',
    'string': 'green',
    'date': 'magenta',
    // "name": intentionally not styling
    'regexp': 'red'
};

function coloredString(str, style)
{
    style = exports.inspect.styles[style];
    if (!style) return str;

    return '\u001b[' + exports.inspect.colors[style][0] + 'm' + str +
           '\u001b[' + exports.inspect.colors[style][1] + 'm';
}

function simpleString(str, style)
{
    return str;
}

/**
@function format(fmt [, arg_1 .. arg_n])
@param fmt {String}
@param [args]*
@returns {String}

Return a formatted string using `fmt` string similar to `printf`.
Format flag is prefixed with `%` sign. Allowed format flags are:

  * `%`             Print single percent
  * `s`             Print next argument as a String
  * `d`, `i`, `f`   Print next argument as a Number (both integer and float). Width and precion options are not allowed
  * `j`             Print next argument as a JSON string
  * `o`, `O`        Print next argument as an Object using #inspect

If there is no more argument for a flag, the flag is not replaced:

    util.format('%s:%d', 'aaa'); // 'aaa:%d'

If there are more arguments than flags in the `fmt`, these extra arguments are
converted to strings using #inspect and these strings are concatenated,
delimited by space:

    util.format('%d-%d', 1, 2, 3, 4) // '1-2 3 4'

If `fmt` is not a string, all arguments  are converted to strings using
#inspect and these strings are concatenated, delimited by space:

    util.format(1, 2, 3, 4) // '1 2 3 4'
**/
exports.format = function(fmt)
{
    var result = '';
    var args = arguments;
    var i = 0, len = args.length;

    if (typeof fmt === 'string')
    {
        i = 1;
        result = String(fmt).replace(/%[%sdifjoO]/g, function(x)
            {
                if (i >= len) return x;
                switch (x)
                {
                case '%%':
                    return '%';
                case '%s':
                    return String(args[i++]);
                case '%d': case '%i': case '%f':
                    return Number(args[i++]);
                case '%j':
                    return JSON.stringify(args[i++]);
                case '%o': case '%O':
                    return exports.inspect(args[i++]);
                default:
                    return x;
                }
            });
    }
    for (; i < len; ++i)
    {
        if (result) result += ' ';
        result += exports.inspect(args[i]);
    }
    return result;
};

function formatValue(ctx, value, recurseDepth)
{
    var primitive = formatPrimitive(ctx, value);
    if (primitive) return primitive;

    var base = '', is_array = false, is_regex = false, braces = ['{', '}'];

    var keys = Object.keys(value);
    var visibleKeys = {};
    keys.forEach(function(name) { visibleKeys[name] = true; }); 

    if (ctx.showHidden) keys = Object.getOwnPropertyNames(value);

    if (exports.isArray(value))
    {
        is_array = true;
        braces = ['[', ']'];
    }
    if (typeof value === 'function')
    {
        var name = value.name ? ': ' + value.name : '';
        base = '[Function' + name + ']';
        if (keys.length === 0) return ctx.stylize(base, 'special');
        else base = ' ' + base;
    }
    if (exports.isRegExp(value))
    {
        is_regex = true;
        base = RegExp.prototype.toString.call(value);
        if (keys.length === 0) return ctx.stylize(base, 'regexp');
        else base = ' ' + base;
    }
    if (exports.isDate(value))
    {
        base = Date.prototype.toUTCString.call(value);
        if (keys.length === 0) return ctx.stylize(Date.prototype.toString.call(value), 'date');
        else base = ' ' + base;
    }
    if (exports.isError(value))
    {
        base = '[' + Error.prototype.toString.call(value) + ']';
        if (keys.length === 0) return ctx.stylize(base, 'error');
        else base = ' ' + base;
    }

    if (keys.length === 0 && (!is_array || value.length == 0))
    {
        return braces[0] + base + braces[1];
    }

    if (recurseDepth < 0)
    {
        return is_regex? ctx.stylize(base, 'regexp') : ctx.stylize('[Object]', 'special');
    }

    ctx.visited.push(value);
    var output = is_array? formatArray(ctx, value, recurseDepth, visibleKeys, keys)
        : keys.map(function(key) { return formatProperty(ctx, value, recurseDepth, visibleKeys, key, is_array); });
    ctx.visited.pop();

    return reduceToSingleString(output, base, braces);
}

function formatPrimitive(ctx, value)
{
    if (value === null) return ctx.stylize('null', 'null');
    switch (typeof value)
    {
    case 'undefined':
        return ctx.stylize('undefined', 'undefined');
    case 'string':
        var simple = '\'' + JSON.stringify(value).replace(/^"|"$/g, '').replace(/'/g, "\\'").replace(/\\"/g, '"') + '\'';
        return ctx.stylize(simple, 'string');
    case 'number':
        return ctx.stylize('' + value, 'number');
    case 'boolean':
        return ctx.stylize('' + value, 'boolean');
    }
    return undefined;
}

function formatArray(ctx, value, recurseDepth, visibleKeys, keys)
{
    var output = [];
    for (var i = 0; i < value.length; ++i)
    {
        var idx = String(i);
        output.push(hasOwnProperty(value, idx)? formatProperty(ctx, value, recurseDepth, visibleKeys, idx, true) : '');
    }

    keys.forEach(function(key)
    {
        if (!key.match(/^\d+$/))
        {
            output.push(formatProperty(ctx, value, recurseDepth, visibleKeys, key, true));
        }
    });

    return output;
}

function formatProperty(ctx, value, recurseDepth, visibleKeys, key, is_array)
{
    var name, str;

    var desc = Object.getOwnPropertyDescriptor(value, key) || { value: value[key] };
    if (desc.get)
    {
        str = ctx.stylize(desc.set? '[Getter/Setter]' : '[Getter]', 'special');
    }
    else if (desc.set)
    {
        str = ctx.stylize('[Setter]', 'special');
    }
    else if (ctx.visited.indexOf(desc.value) > -1)
    {
        str = ctx.stylize('[Circular]', 'special');
    }
    else
    {
        str = formatValue(ctx, desc.value, recurseDepth === null? null : recurseDepth - 1);
        if (str.indexOf('\n') > -1)
        {
            str = str.split('\n').map(function(line){ return (is_array? '  ' : '   ') + line; }).join('\n');
            str =  is_array? str.substr(2) : '\n' + str;
        }
    }

    if (hasOwnProperty(visibleKeys, key)) 
    {
        if (is_array && key.match(/^\d+$/))
        {
            return str;
        }

        name = JSON.stringify('' + key);
        if (name.match(/^"([a-zA-Z_][a-zA-Z_0-9]*)"$/))
        {
            name = name.substr(1, name.length - 2);
            name = ctx.stylize(name, 'name');
        }
        else
        {
            name = name.replace(/'/g, "\\'").replace(/\\"/g, '"').replace(/(^"|"$)/g, "'");
            name = ctx.stylize(name, 'string');
        }
    }
    else
    {
        name = '[' + key + ']';
    }

    return name + ': ' + str;
}

function reduceToSingleString(output, base, braces)
{
    var num_lines = 0;
    var length = output.reduce(function(prev, current)
        {
            ++num_lines;
            if (current.indexOf('\n') >= 0) ++num_lines;
            return prev +current.length + 1;
        }, 0);

    var payload;
    if (length > 60) //TODO: console window width
    {
        payload = (base === '' ? '' : base + '\n ') + ' ' + output.join(',\n  ') + ' ';
    }
    else
    {
        payload = base + ' ' + output.join(', ') + ' ';
    }
    return braces[0] + payload + braces[1];
}
