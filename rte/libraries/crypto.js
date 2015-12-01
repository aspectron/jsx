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
@module crypto - Crypto
Cryptographic functions and classes.
**/

exports.$ = rt.bindings.crypto;

/**
@function createHash(algorithm)
@param algorithm {String}
@return {Hash}
Create and initiaize generator with specified algorithm, see #getHashes, #Hash
**/
exports.$.createHash = function createHash(algorithm)
{
	return new rt.bindings.crypto.Hash(algorithm);
}

/**
@function hashDigest(algorithm, data [, hex=true])
@param algorithm {String}
@param data {String|Buffer}
@param [hex=true] {Boolean}
@return {String}
Return digest string for specified hash `algorithm` and `data` string or buffer.
Hash algorithm is one of values return by #getHashes(), for example:
  * `sha1`
  * `sha256`
  * `sha512`

See also #Hash
**/
exports.$.hashDigest = function hashDigest(algorithm, data, hex)
{
	return exports.$.createHash(algorithm).update(data).digest(hex || true);
}

/**
@function createHmac(algorithm, key)
@param algorithm {String}
@param key {String|Buffer}
@return {Hmac}
Create and initiaize generator with specified algorithm and key, see #getHashes, #Hmac
**/
exports.$.createHmac = function createHmac(algorithm, key)
{
	return new rt.bindings.crypto.Hmac(algorithm, key);
}

/**
@function hmacDigest(algorithm, data, key [, hex=true])
@param algorithm {String}
@param data {String|Buffer}
@param key {String|Buffer}
@param [hex=true] {Boolean}
@return {String}
Return HMAC digest string for specified hash `algorithm` and `data` string or buffer
using `key` string or buffer.
Hash algorithm is one of values return by #getHashes(), for example:
  * `sha1`
  * `sha256`
  * `sha512`

See also #Hmac
**/
exports.$.hmacDigest = function hmacDigest(algorithm, data, key, hex)
{
	return exports.$.createHmac(algorithm, key).update(data).digest(hex || true);
}

/**
@function createCipher(algorithm, password [, salt [, iterations [, hash]]])
@param algorithm {String}
@param password {String|Buffer}
@param [salt] {String|Buffer}
@return {Cipher}
Create new #Cipher object for data encryption.
Array of supported cipher algorithms is return by #getCiphers() function.
`password` and optional `salt` arguments are used to generate key and initilization vector,
see #Cipher.generateKey() for details.
**/
exports.$.createCipher = function createCipher(algorithm, password)
{
	var cipher = new rt.bindings.crypto.Cipher(algorithm, true);
	return cipher.init.apply(cipher, [].slice.call(arguments, 1));
}

/**
@function createCipheriv(algorithm, key, iv)
@param algorithm {String}
@param key {String|Buffer}
@param iv {String|Buffer}
@return {Cipher}
Create new #Cipher object for data encryption.
Array of supported cipher algorithms is return by #getCiphers() function.
`key` and `iv` arguments are used to initialize the cipher,
see #Cipher.generateKey() for details.
**/
exports.$.createCipheriv = function createCipheriv(algorithm, key, iv)
{
	var cipher = new rt.bindings.crypto.Cipher(algorithm, true);
	return cipher.initiv(key, iv);
}

/**
@function createDecipher(algorithm, password [, salt [, iterations [, hash]]])
@param algorithm {String}
@param password {String|Buffer}
@param [salt] {String|Buffer}
@return {Cipher}
Create new #Cipher object for data decryption.
Array of supported cipher algorithms is return by #getCiphers() function.
`password` and optional `salt` arguments are used to generate key and initilization vector,
see #Cipher.generateKey() for details.
**/
exports.$.createDecipher = function createDecipher(algorithm, password)
{
	var cipher = new rt.bindings.crypto.Cipher(algorithm, false);
	return cipher.init.apply(cipher, [].slice.call(arguments, 1));
}

/**
@function createDecipheriv(algorithm, key, iv)
@param algorithm {String}
@param key {String|Buffer}
@param iv {String|Buffer}
@return {Cipher}
Create new #Cipher object for data decryption.
Array of supported cipher algorithms is return by #getCiphers() function.
`key` and `iv` arguments are used to initialize the cipher,
see #Cipher.generateKey() for details.
**/
exports.$.createDecipheriv = function createDecipheriv(algorithm, key, iv)
{
	var cipher = new rt.bindings.crypto.Cipher(algorithm, false);
	return cipher.initiv(key, iv);
}

/**
@function encrypt(algorithm, data, key)
@param algorithm {String}
@param data {String|Buffer}
@return {Buffer}
Return buffer encrypted with specified `algorithm` for `data` and `key` string or buffer.
Encryption algorithm is one values return by #getCiphers(), for example:
  * `aes128`,
  * `aes192`,
  * `aes256`,
  * `blowfish`,
  * `idea`
**/
exports.$.encrypt = function encrypt(algorithm, data, key)
{
	return exports.$.createCipher(algorithm, key).update(data).final();
}

/**
@function decrypt(algorithm, data, key)
@param algorithm {String}
@param data {String|Buffer}
@return {Buffer}
Return buffer decrypted with specified `algorithm` for `data` and `key` string or buffer.
Decryption algorithm is one values return by #getCiphers(), for example:
  * `aes128`,
  * `aes192`,
  * `aes256`,
  * `blowfish`,
  * `idea`
**/
exports.$.decrypt = function decrypt(algorithm, data, key)
{
	return exports.$.createDecipher(algorithm, key).update(data).final();
}

/**
@function createDiffieHellman(prime [, generator = 2])
@param prime {Number|Buffer}
@param [generator=2] {Number|Buffer}
@return {DiffieHellman}
Create a Diffie-Hellman key exchange.
When `prime` is a `Number`, generate a prime with specified bit length.
When `prime` is a `Buffer`, use it as a prime number.
Optional `generator` argument could be a `Number` or `Buffer`, otherwise generator value `2` is used.
See #DiffieHellman class.
**/
exports.$.createDiffieHellman = function createDiffieHellman(prime, generator)
{
	return new rt.bindings.crypto.DiffieHellman(prime, generator);
}

/**
@function getDiffieHellman(group_name)
@param group_name {String}
@return {DiffieHellman}
Create a Diffie-Hellman key exchange instance from predefined group name.
The supported groups are defined in [RFC 2412](http://www.rfc-editor.org/rfc/rfc2412.txt)
and [RFC 3526](http://www.rfc-editor.org/rfc/rfc3526.txt):
  * `modp1`
  * `modp2`
  * `modp5`
  * `modp14`
  * `modp15`
  * `modp16`
  * `modp17`
  * `modp18`

See #DiffieHellman class.
**/
exports.$.getDiffieHellman = function getDiffieHellman(group_name)
{
	return new rt.bindings.crypto.DiffieHellman(group_name);
}
