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

var mongodb = require("mongodb");

var client = new mongodb.Client();
client.connect("localhost", function(err, client)
{
	if (err) throw err;

    var db = client.getDatabase("testdb");
    var coll = db.collection("collection");

    console.log("database names:", client.databaseNames());
    console.log("collection names:", db.collectionNames());

    // insertion
    coll.insert({a : 'text', b : 123, c : 3.14, d : [1, 2, 3]}, function(err)
    {
        if (err) throw err;

        console.log("doc inserted");

        coll.insert([{name: '1', value: 1}, {name: '2', value: 2}, {name: '3', value: 3}], function(err)
        {
            if (err) throw err;

            console.log("array of docs inserted");

            // find
            coll.find(function(err, result)
                {
                    if (err) throw err;
                    console.log("find all:", result.toArray());
                });

            // count
            coll.count({name: '1'}, function(err, result)
                {
                    if (err) throw err;
                    trace("count of {name: 1} :", result);
                }); 

            // find and setup cursor
            var result = coll.find().limit(2).skip(1).toArray();
            console.log("find all (limit 2, skip 1):", result);

            coll.find({name: '1'}, { name: '1', value: 1}, { limit: 10, skip: 0 }, function(err, cursor)
            {
                if (err) throw err;

                console.log("async find:", cursor.toArray());
    
                var doc = coll.findOne({name: '2'});
                console.log("find one:", doc);

                // indexes
                coll.ensureIndex({name: 1}, function(err, result)
                {
                    if (err) throw err;

                    console.log("index created:", result);
                    coll.indexInfo(function(err, cursor)
                        {
                            if (err) throw err;
                            console.log("indexes:", cursor.toArray());
                        });
                });

                // modification
                coll.update({name: '1'}, {$set:{value: 99}}, {upsert: true, multi: true}, function(err)
                {
                    if (err) throw err;

                    console.log("updated:", coll.find({name: '1'}).toArray());

                    coll.remove({name: '1'}, function(err)
                    {
                        if (err) throw err;

                        console.log("removed:", coll.find({name: '1'}).toArray());

                        coll.drop(function(err, info)
                        {   
                            if (err) throw err;

                            console.log("collection dropped. info:", info);

                            db.drop(function(err, info)
                            {
                                if (err) throw err;
                                console.log("database dropped. info:", info);
                            });
                        });
                    });
                });
            });
        });
    });
});
