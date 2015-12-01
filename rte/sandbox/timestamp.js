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
var session = new db.Session("test.db");

session.exec("drop table if exists test");
session.exec("create table test(id int)");

var ins = session.prepare("insert into test(id) values(?)");

var timestamp = Date.parse("2011-12-12 21:21:21");
trace(timestamp)
session.begin(); // start transaction
ins.bind(timestamp/1000);
ins.exec();

session.commit();
var sel = session.prepare("select * from test ");
var res = sel.query();
while ( res.next() )
{
    trace(res.value('id')+"----------");
}

