//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var crypto = require('crypto');
var util = require('util');

console.log(crypto);

var hash = crypto.createHash('sha1');

hash.update('aa').update('bb').update('zzz');
console.log('HASH', hash.algorithm(), hash.digest());
console.log('HASH', hash.algorithm(), hash.reset().digest());

var hmac = crypto.createHash('sha512');

hmac.update('1').update('2').update('3');
console.log('HMAC', hmac.algorithm(), hmac.digest());
console.log('HMAC', hmac.algorithm(), hmac.reset().digest());

function cipherInfo(cipher)
{
	return util.format('%s key_len=%d iv_len=%d block_size=%d',
		cipher.algorithm(), cipher.keySize(), cipher.ivSize(), cipher.blockSize());
}

function keyInfo(info)
{
	return util.format('key=%s iv=%s', info.key.toString('hex'), info.iv.toString('hex'));
}

var encrypt = new crypto.Cipher('aes128', true);
console.log('cipher:', cipherInfo(encrypt));

encrypt.setAutoPadding();
encrypt.setAutoPadding(false);

console.log('generated key1:', keyInfo(encrypt.generateKey('password')));
console.log('generated key2:', keyInfo(encrypt.generateKey('password', 'salt', 20, 'sha1')));
console.log('generated key3:', keyInfo(encrypt.generateKey({ password: 'secret', salt: '123', iterations: 3, hash: 'sha512'})));

var cipher = encrypt.init({password: 'secret', salt: '1234'}).update('data').update(' more data').final();
console.log('cipher: ', cipher.toString('hex'));

var decrypt = new crypto.Cipher('aes128', false);
var plain = decrypt.init({password: 'secret', salt: '1234'}).update(cipher).final();
console.log('plain: ', plain.toString());

if (rt.local.scriptArgs.indexOf('--test-hashes') != -1)
{
	var data = 'text', key = 'key';
	crypto.getHashes().forEach(function(type)
	{
		console.log('`%s` %s hash digest: %s', data, type, crypto.hashDigest(type, data));
		console.log('`%s` %s HMAC digest: %s', data, type, crypto.hmacDigest(type, data, key));
	});
}

if (rt.local.scriptArgs.indexOf('--test-ciphers') != -1)
{
	crypto.getCiphers().forEach(function(type)
	{
		var info = cipherInfo(crypto.createCipher(type, ''));

		var data = 'text', key = 'key';
		var cipher, plain, auth_tag;
		try
		{
			if (type.toUpperCase().indexOf('-XTS') != -1)
			{
				// XTS ciphers require data at least 16 byte length
				data = data + data;
				data = data + data;
			}

			if (type.toUpperCase().indexOf('-GCM') != -1)
			{
				// GCM mode requires authentication tag 
				// get it after encryption and set before decryption

				encrypter = crypto.createCipher(type, key);
				cipher = encrypter.update(data).final();
				auth_tag = encrypter.getAuthTag();

				decrypter = crypto.createDecipher(type, key);
				decrypter.setAuthTag(auth_tag);
				plain = decrypter.update(cipher).final();
			}
			else
			{
				cipher = crypto.encrypt(type, data, key);
				plain = crypto.decrypt(type, cipher, key);
			}
		}
		catch (e)
		{
			plain = e;
		}
		console.log('%s encrypt/decrypt: %s', info, plain);
	});
}

function test_dh(dh1, dh2)
{
	console.log('Generating keys...');
	dh1.generateKeys();
	dh2.generateKeys();

	console.log('prime1:', dh1.getPrime().toString('hex'));
	console.log('generator1:', dh1.getGenerator().toString('hex'));

	console.log('prime2:', dh2.getPrime().toString('hex'));
	console.log('generator2:', dh2.getGenerator().toString('hex'));

	console.log('public  key1:', dh1.getPublicKey().toString('hex'));
	console.log('private key1:', dh1.getPrivateKey().toString('hex'));

	console.log('public  key2:', dh2.getPublicKey().toString('hex'));
	console.log('private key2:', dh2.getPrivateKey().toString('hex'));

	var secret1 = dh1.computeSecret(dh2);
	var secret2 = dh2.computeSecret(dh1);
	console.log('shared secret1: ', secret1.toString('hex'));
	console.log('shared secret2: ', secret2.toString('hex'));
}

var prime_len = 123, generator = 5;
console.log('Creating DiffieHellman prime %d bits, generator=%d', prime_len, generator);

var dh1 = new crypto.DiffieHellman(prime_len, generator);
var dh2 = new crypto.DiffieHellman(dh1.getPrime(), dh1.getGenerator());
test_dh(dh1, dh2);

dh1.setPublicKey(dh2.getPublicKey());
dh1.setPrivateKey(dh2.getPrivateKey());
console.log('set public  key1:', dh1.getPublicKey().toString('hex'));
console.log('set private key1:', dh1.getPrivateKey().toString('hex'));

if (rt.local.scriptArgs.indexOf('--test-dhgroups') != -1)
{
	crypto.DiffieHellman.modGroups().forEach(function(group){

		console.log('Creating Diffie-Hellman group', group);
		var dh1 = new crypto.DiffieHellman(group);
		var dh2 = new crypto.DiffieHellman(group);

		test_dh(dh1, dh2);
	});
}

console.log('PBKDF2:', crypto.PBKDF2('pass', 'salt', 13, 256/8, 'md5').toString('hex'));
console.log('random:', crypto.randomBytes(256/8).toString('hex'));
console.log('pseudo random:', crypto.pseudoRandomBytes(256/8).toString('hex'));
