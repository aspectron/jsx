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

var db = require("db");

function fill_db(session)
{
    console.log("dropping table test");
	session.exec("drop table if exists test");
    console.log("creating table test");
	session.exec("create table test(id integer primary key autoincrement, str text, data blob)");

	var ins = session.prepare("insert into test(id, str, data) values(?, ?, ?)");

	session.begin(); // start transaction

	for (var i = 0; i < 10; ++i)
	{
	    ins.bind(i, "str" + i, null);
	    ins.exec();

	    var params = [i + 20, "str" + (i + 20), null];
	    ins.bind(params);
	    ins.exec();
	}
	session.commit();
}

function print_db(session)
{
    var sel = session.prepare("select * from test order by id");
    console.log("data:", sel.query().asArrayOfArrays());
}

var session = new db.Session();

session.open("sqlite3:db=test.db", function(error)
{
    if (error) throw error;

    console.log("opened ", this);
    fill_db(this);

    this.prepare("select * from test where (id > ? and str > ?)", 5, "str", function(error, statement)
    {
        if (error) throw error;

        statement.query().asArrayOfArrays(function(error, rows)
        {
            if (error) throw error;
            console.log("arrays:", rows);
        });

        statement.query().asArrayOfObjects(3, function(error, rows)
        {
            if (error) throw error;
            console.log("objects:", rows);
        });

        session.close(function(error)
        {
            if (error) throw error;

            console.log("closed", this);
            rt.exit();
        });
    });
});

/*
session.exec("insert into test (id, str, data) values(?, ?, ?)", null, "data 99", "blob data",
    function(error, info, rows)
    {
        if (error) throw error;
        console.log("insert exec async\n\tinfo:%o\n\trows:%o", info, rows);
    });

session.exec("select * from test where id between ? and ?", 0, 100,
    function(error, info, rows)
    {
        if (error) throw error;
        console.log("select exec async\n\tinfo:%o\n\trows:%o", info, rows);
    });

session.exec("delete from test where id < ?", 10,
    function(error, info, rows)
    {
        if (error) throw error;
        console.log("delete exec async\n\tinfo:%o\n\trows:%o", info, rows);
        print_db(this);
    });
*/