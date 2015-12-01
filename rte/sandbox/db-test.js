//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var db = require("db");
var session = new db.Session("test");

session.exec("drop table if exists test");
session.exec("create table test(id int, str text, data blob)");

var ins = session.prepare("insert into test(id, str, data) values(?, ?, ?)");

session.begin(); // start transaction

for (var i = 0; i < 10; ++i)
{
    ins.bind(i, "str" + i, null);
    ins.exec();

    ins.bind([i + 20, "str" + (i + 20), null]);
    ins.exec();
}
session.commit();

var sel = session.prepare("select * from test where id > ? order by id", 1);
var res = sel.query();

var names = [];
for (var i = 0; i < res.cols(); ++i)
{
    names.push(res.name(i));
}
console.log("column names:", names);

while ( res.next() )
{
    var row = [];
    for (var i = 0; i < res.cols(); ++i)
    {
        row.push(res.value(i));
    }
    console.log("values column index:", row);
    console.log("values column name: ", res.value("id") + " = " + res.value("str"));
    
    console.log("values array:       ", res.asArray());
    console.log("values object:      ", res.asObject());
}

console.log("result array of arrays: ", sel.query().asArrayOfArrays());
console.log("result array of objects:", sel.query().asArrayOfObjects(5));
