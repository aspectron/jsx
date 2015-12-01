//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
function assert_eq(msg, actual, expected)
{
    if ( actual != expected )
    {
        console.error('%s: expected `%o` actual `%o`', msg, expected, actual);
    }
}


var buf = new Buffer('text');

buf.dump(12);

assert_eq('string', buf.string(), 'text');
assert_eq("hex", Buffer.fromString(buf.toString('hex'), 'hex').string(), 'text');

['sha1', 'sha256', 'sha512'].forEach(function(type)
	{
		console.log("%s hash digest: %s", type, buf.hash(type));
		console.log("%s hmac digest: %s", type, buf.hmac(type, 'key'));
	});

assert_eq('substring(0)', buf.substring(0), 'text');
assert_eq('substring(2)', buf.substring(2), 'xt');
assert_eq('substring(2, 99)', buf.substring(2, 99), 'xt');
assert_eq('substring(0, 4)', buf.substring(0, 4), 'text');

assert_eq('base64', Buffer.fromString(Buffer.fromString('1').toString('base64'), 'base64').string(), '1');
assert_eq('base64', Buffer.fromString(Buffer.fromString('12').toString('base64'), 'base64').string(), '12');
assert_eq('base64', Buffer.fromString(Buffer.fromString('123').toString('base64'), 'base64').string(), '123');
assert_eq('base64', Buffer.fromString(Buffer.fromString('1234').toString('base64'), 'base64').string(), '1234');

assert_eq('compress', buf.compress().decompress().string(), 'text');
assert_eq('encrypt', buf.encrypt('key').decrypt('key').string(), 'text');

buf = new Buffer;
buf.appendUint32(1);
buf.appendUint32(2);
assert_eq('uint32(0)', buf.getUint32(0), 1);
assert_eq('uint32(4)', buf.getUint32(4), 2);
assert_eq('change uint32', buf.setUint32(4, 0).getUint32(4), 0);
assert_eq('uint32(0)', buf.getUint32(0), 1);

buf.appendInt8(0x22);
buf.dump();