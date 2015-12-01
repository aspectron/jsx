//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var fs = require('fs'),
    crypto = require('crypto');

var target_folder = 'rte-release';

function encrypt_script(source, target)
{
    var buffer_in = fs.readFile(source);
    if (!buffer_in)
        throw new Error('Unable to read file ' + source);

    var buffer_out = crypto.encryptScript(buffer_in);
    fs.writeFile(target, buffer_out);
    console.log('Encrypt %s to %s', source, target);
}

function encrypt_directory(relative_path_to_rte)
{
    var files = [];
    fs.enumerateDirectory(rt.local.rtePath + '/' + relative_path_to_rte,
        function(name) 
        {  
            if (fs.isRegularFile(name) && name.extension().toLowerCase() === '.js')
                files.push(name);
        });

    var out = rt.local.rootPath + '/' + target_folder + '/' + relative_path_to_rte;
    fs.createDirectoryRecursive(out);

    for(var i = 0; i < files.length; i++)
    {
        var source = files[i];
        var target = out + '/' + source.filename();
        encrypt_script(source,target);
    }
}

fs.remove(rt.local.rootPath + '/' + target_folder, true);
encrypt_directory('libraries');
