//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
pragma("event-queue");

var fs = require("fs");

function assert_eq(msg, actual, expected)
{
    if ( actual != expected )
    {
        console.log('%s: expected: %o actual: %o', msg, expected, actual);
    }
}

///////////////////////////////////////////////////////////
//
console.log("test path");

var PATH_STR = "ай/foo/bar.txt";
var path = new fs.Path(PATH_STR);

assert_eq("string", path, PATH_STR);
assert_eq("stem", path.stem(), "bar");
assert_eq("filename", path.filename(), "bar.txt");
assert_eq("extension", path.extension(), ".txt");

var path2 = new fs.Path();
assert_eq("empty ctor", path2, "");

var path3 = new fs.Path(path)
assert_eq("path ctor", path3, path.toString());

assert_eq("resolve path", fs.resolve("a/b/../.././c"), "c");

var abs_path = new fs.Path(fs.currentDirectory() + "/a/b/c");
assert_eq("absolute path", fs.absolute("a/b/c"), abs_path);

console.log("current directory:", fs.currentDirectory());

///////////////////////////////////////////////////////////
//
var FILENAME = "test-нелатин.txt";
var FILEDATA = "данные в файле 1111.";
console.log("file operations\n");
assert_eq("write", fs.writeFile(FILENAME, FILEDATA), true);
assert_eq("read", fs.readFile(FILENAME).string(), FILEDATA);
assert_eq("is_directory", fs.isDirectory(FILENAME), false);
assert_eq("is_regular_file", fs.isRegularFile(FILENAME), true);

var FILENAME2 = "2" + FILENAME;
if ( fs.isRegularFile(FILENAME2) )
{
    fs.remove(FILENAME2);
}
fs.copy(FILENAME, FILENAME2);
assert_eq("read copied file", fs.readFile(FILENAME2).string(), FILEDATA);

fs.remove(FILENAME);
assert_eq("is_regular_file for removed file", fs.isRegularFile(FILENAME), false);

var FILENAME3 = "3" + FILENAME;
fs.rename(FILENAME2, FILENAME3);
assert_eq("is_regular_file for renamed old file", fs.isRegularFile(FILENAME2), false);
assert_eq("is_regular_file for renamed new file", fs.isRegularFile(FILENAME3), true);
fs.remove(FILENAME3);

///////////////////////////////////////////////////////////
//
console.log("directory operations\n");

var DIRNAME = "test-dir-нелатин";
fs.createDirectory(DIRNAME);
assert_eq("is_directory", fs.isDirectory(DIRNAME), true);
assert_eq("is_regular_file", fs.isRegularFile(DIRNAME), false);

console.log("test enumerate_directory\n");
assert_eq("write empty file", fs.writeFile(DIRNAME + "/qqq.txt", ""), true);
assert_eq("read empty file", fs.readFile(DIRNAME + "/qqq.txt").string(), "");
fs.enumerateDirectory(DIRNAME, function(path) { console.log("%s", path); });

///////////////////////////////////////////////////////////
//
if (!fs.DirectoryMonitor)
{
    rt.exit();
}
else
{
    console.log("directory monitor\n");

    var monitor = new fs.DirectoryMonitor;
    var monitor2 = new fs.DirectoryMonitor;

    monitor.add(DIRNAME, true, "zzz.*");
    monitor.add(".", true, ".*zzz2.*");

    console.log('watch dirs:', monitor.dirs());

    monitor.monitor(
        function(err, ev)
        {
            if (err) throw err;

            var type_str = ["null", "added", "removed", "modified", "renamed_old_name", "renamed_new_name"];
            console.log(ev.path + " " + type_str[ev.type]);
        });
        
    fs.writeFile(DIRNAME + "/qq.txt", "data");
    fs.writeFile(DIRNAME + "/zzz.txt", "data");
    fs.writeFile(DIRNAME + "/zzz.txt", "data data data", true);
    fs.rename(DIRNAME + "/zzz.txt", DIRNAME + "/zzz2.txt");
    fs.remove(DIRNAME + "/zzz2.txt");

    setTimeout(function() {
        console.log('Stop monitor');
        monitor.stop();
        rt.exit(0);
    }, 1000);
}
