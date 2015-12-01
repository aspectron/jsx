//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#include "jsx/http.hpp"

#include "jsx/runtime.hpp"
#include "jsx/http_client.hpp"
#include "jsx/http_server.hpp"
#include "jsx/http_logger.hpp"
#include "jsx/websocket.hpp"
#include "jsx/url.hpp"

namespace aspect { namespace http {

void resolveAbsoluteLocation(v8::FunctionCallbackInfo<v8::Value> const& args);

void setup_bindings(v8pp::module& target)
{
	/**
	@module http HTTP
	HTTP native bindings
	**/
	v8pp::module http_module(target.isolate());

	/**
	@class Client
	HTTP client implementation
	**/
	v8pp::class_<client> client_class(target.isolate(), v8pp::v8_args_ctor);
	client_class
		/**
		@function connect(url, num_tries)
		@param url {String}
		@param num_tries {Number}
		Connect to `url` with maximum number of tries `num_tries`
		**/
		.set("connect", &client::connect)

		/**
		@function close()
		Close connection
		**/
		.set("close", &client::close)

		/**
		@function reset()
		Reset connection
		**/
		.set("reset", &client::reset)

		/**
		@function setKeepAlive(value)
		@param value {Boolean}
		Set keep alive flag for connection
		**/
		.set("setKeepAlive", &client::set_keep_alive)

		/**
		@function ajax(options)
		@param options {Object}
		@return {Client} `this` to chain calls
		Perform AJAX request with specified `options`
		Options could have following attributes:
		  * `async`      Boolean flag to use asynchronous request, default `true`
		  * `url`        Request URL string
		  * `retries`    Number of tries
		  * `type`       Request method type: `GET`, `POST`
		  * `resource`   Resource string
		  * `headers`    HTTP headers object
		  * `data`       Query string for the request type `GET`, content string for the request type `POST`
		**/
		.set("ajax", &client::ajax)
		;

	/**
	@class Server
	HTTP server implementation

	@function Server(port)
	@param port {Number}
	@return {Server}
	Create an HTTP server instance with `port` used to listen for new connections
	**/
	v8pp::class_<server> server_class(target.isolate(), v8pp::v8_args_ctor);
	server_class
		/**
		@function digest(resource, handler)
		@param resource {String}
		@param handler {Function|Service}
		@return {Server} `this` to chain calls
		Add `handler` function or #Service instance to handle requests for `resource`
		**/
		.set("digest", &server::digest)

		/**
		@function setSslKeyfile(filename)
		@param filename {String}
		@return {Server} `this` to chain calls
		Configure the server for SSL using `filename` certificate
		**/
		.set("setSslKeyfile", &server::set_ssl_keyfile)

		/**
		@function clearCache()
		Clear cache for all services used in the server
		**/
		.set("clearCache", &server::clear_cache)

		/**
		@function start()
		@return {Server} `this` to chain calls
		Start the server
		**/
		.set("start", &server::start)

		/**
		@function stop(wait)
		@param wait {Boolean}
		@return {Server} `this` to chain calls
		Stop the server. Wait for the server stop completion if `wait == true`
		**/
		.set("stop", &server::stop)

		/**
		@function setAuth(options)
		@param options {Object}
		@return {Server} `this` to chain calls
		Set server basic authentication options:
		  * `realm`     Realm string
		  * `restrict`  Restrict options object
		  * `allow`     Allow list

		Restrict options object is sued to map the server resources to users like this:

		    {
		      '/area1': { 'user1' : 'password1', 'user2' : 'password2'},
		      '/area2': { 'usera' : 'passa', 'userb' : 'passb'},
		    }

		Allow list is an array of resources allowed to access without authentication:

		    [ '/public1', '/public2' ]
		**/
		.set("setAuth", &server::set_auth)
		;

	/**
	@class Service
	Base class for HTTP services added to #Server.digest
	**/
	v8pp::class_<service> service_class(target.isolate(), v8pp::no_ctor);

	/**
	@class FunctionService
	HTTP service with callback function handler

	@function FunctionService(handler)
	@param handler {Function}
	@return {FunctionService}
	Create a function service instance with `handler` function callback.

	@event callback(request, writer) FunctionService handler
	@param request {Request}
	@param writer {Writer}
	**/
	v8pp::class_<function_service> function_service_class(target.isolate(), v8pp::no_ctor);
	function_service_class.inherit<service>();

	/**
	@class FilesystemService
	HTTP service for static content in filesystem

	@function FilesystemService(root)
	@param root {String}
	@return {FilesystemService}
	Create a FilesystemService instance with for `root` path in filesystem.
	**/
	v8pp::class_<filesystem_service> filesystem_service_class(target.isolate(), v8pp::v8_args_ctor);
	filesystem_service_class
		.inherit<service>()
		/**
		@function clearCache()
		Clear the service cache
		**/
		.set("clearCache", &filesystem_service::clear_cache)
		/**
		@function setOption(name, value)
		@param name {String}
		@param value {String}
		Set the service option. Allowed options are:
		  * `directory` Set directory name so all files within the directory will be available.

		  * `file`      Set single file that is served by the service

		  * `cache`     Set cache option:
		     * `0`         Do not cache files in memory
		     * `1`         Cache files in memory when requested, check for any updates
		     * `2`         Cache files in memory when requested, ignore any updates

		  * `scan`      Set scan configuration (only applies to directories)
		     * `0`         Do not scan the directory; allow files to be added at any time
		     * `1`         Scan directory when started, and do not allow files to be added
		     * `2`         Scan directory and pre-populate cache; allow new files
		     * `3`         Scan directory and pre-populate cache; ignore new files

		  * `max_cache_size`  Maximum cache size in bytes. Files larger than this size
		  will never be cached in memory. A value of zero means that the size is unlimited.

		  * `max_chunk_size`  Maximum chunk size in bytes. Files larger than this size
		  will be delivered to clients using HTTP chunked responses. A value of zero means
		  that the size is unlimited (chunking is disabled).

		  * `writable`  Whether the file and/or directory served are writable, `true` or `false`
		  * `upload_directory`  Set directory name for files uploaded to the server
		**/
		.set("setOption", &filesystem_service::set_option)
		;

	//v8pp::class_<external_service> external_service_class;
	//external_service_class.inherit<http_service>();

	/**
	@class WebsocketService
	Websocket service. Derived from events#EventEmitter

	When emit events, `this` is a #Websocket instance.

	@event connect(resource)               fired on client connect
	@param resource {String}  HTTP server resource that client is connected to
	@event close                           fired on socket close
	@event data(dataReceived, isBinary)    fired on data received
	@param dataReceived {String|Buffer}      data received
	@param isBinary {Boolean}                `true` if dataReceived is Buffer
	@event error(message)                  fired on socket error
	@param message {String}
	**/
	v8pp::class_<websocket_service> websocket_service_class(target.isolate(), v8pp::v8_args_ctor);
	websocket_service_class
		.inherit<service>()
		.set("on", &websocket_service::on)
		;

	/**
	@class Websocket
	Websocket implementation. Derived from events#EventEmitter

	@event close                           fired on socket close
	@event data(dataReceived, isBinary)    fired on data received
	@param dataReceived {String|Buffer}      data received
	@param isBinary {Boolean}                `true` if dataReceived is Buffer
	@event error(message)                  fired on socket error
	@param message {String}
	**/

	/**
	@function Websocket(url)
	@param url {String}
	@return {Websocket}
	Create a Websocket instance and connect it to specified `url`
	**/
	v8pp::class_<websocket> websocket_class(target.isolate(), v8pp::v8_args_ctor);
	websocket_class
		.inherit<v8_core::event_emitter>()
		/**
		@function send(data)
		@param data {String|Buffer}
		Send data string or binary buffer via the websocket
		**/
		.set("send", &websocket::send)
		/**
		@function close()
		Close the websocket
		**/
		.set("close", &websocket::close)
		;

	// See actual Request object assembly in function_service::impl::process()
	/**
	@class Request - HTTP request
	@property version {String} - HTTP protocol version
	@property method {String} - HTTP protocol method
	@property resource {String} - Requested resource
	@property queryString {String} - Query as a string
	@property query {Object} - Query as an object
	@property headers {Object} - HTTP headers as an object
	@property cookies {Object} - Request cookies as an object
	@property ip {String} - Client IP address
	@property content {Buffer} - Optional HTTP Content as a Buffer
	**/
	v8pp::class_<request> request_class(target.isolate(), v8pp::no_ctor);

	/**
	@class Response
	HTTP response
	**/
	v8pp::class_<response> response_class(target.isolate(), v8pp::no_ctor);
	response_class
		/**
		@function setStatusCode(code)
		@param code {Number}
		Set HTTP status code
		**/
		.set("setStatusCode", &response::set_status_code)

		/**
		@function setStatusMessage(message)
		@param message {String}
		Set HTTP status message
		**/
		.set("setStatusMessage", &response::set_status_message)

		/**
		@function setContentType(type)
		@param type {String}
		Set content type
		**/
		.set("setContentType", &response::set_content_type)

		/**
		@function setLastModified(ts)
		@param ts {Number} Unix timestamp
		Sets the time that the response was last modified (Last-Modified)
		**/
		.set("setLastModified", &response::set_last_modified)

		/**
		@function setHeader(name, value)
		@param name {String}
		@param value {String}
		Add or change HTTP header
		**/
		.set("setHeader", &response::set_header)

		/**
		@function deleteHeader(name)
		@param name {String}
		Delete HTTP header
		**/
		.set("deleteHeader", &response::delete_header)

		/**
		@function setCookie(name, value, path)
		@param name {String}
		@param value {String}
		@param path {String}
		Add or change HTTP cookie
		**/
		.set("setCookie", &response::set_cookie)

		/**
		@function deleteCooke(name, path)
		@param name {String}
		@param path {String}
		Delete HTTP cookie
		**/
		.set("deleteCookie", &response::delete_cookie)
		;

	/**
	@class Writer
	HTTP response writer
	**/
	v8pp::class_<writer> writer_class(target.isolate(), v8pp::no_ctor);
	writer_class
		/**
		@property response {Response}
		HTTP response
		**/
		.set("response", v8pp::property(&writer::resp))
		/**
		@function write(data)
		@param data {String|Buffer}
		Write `data` as a HTTP response content
		**/
		.set("write", &writer::write)
		/**
		@function send()
		Sends all data buffered as a single HTTP message (without chunking)
		**/
		.set("send", &writer::send)

		/**
		@function close()
		Close HTTP connection
		**/
		.set("close", &writer::close)
		;

	/**
	@class Url
	Url implementation

	@function Url([str])
	@param str {String}
	@return {Url}
	Create and return `Url` instance from optional string parameter.
	**/
	v8pp::class_<url> url_class(target.isolate(), v8pp::v8_args_ctor);
	url_class
		/**
		@function isEmpty()
		@return {Boolean}
		Check is the URL empty
		**/
		.set("isEmpty", &url::empty)

		/**
		@function spec()
		@return {String}
		Return URL as a string
		**/
		.set("spec", &url::to_string)

		/**
		@function toString()
		@return {String}
		Convert URL to string
		**/
		.set("toString", &url::to_string)

		/**
		@function resolve(relative)
		@param relative {String}
		@return {Url}
		Resolve a relative reference against the given URL
		**/
		.set("resolve", &url::resolve)

		/**
		@function scheme()
		@return {String}
		Get scheme part of the URL or empty string
		if there is no scheme in the URL.
		**/
		.set("scheme", &url::scheme)

		/**
		@function userinfo()
		@return {String}
		Get user info part of the URL or empty string
		if there is no user info in the URL.
		**/
		.set("userinfo", &url::userinfo)

		/**
		@function host()
		@return {String}
		Get host part of the URL or empty string
		if there is no host in the URL.
		**/
		.set("host", &url::host)

		/**
		@function port()
		@return {String}
		Get port part of the URL or empty string
		if there is no port in the URL.
		**/
		.set("port", &url::port)

		/**
		@function path()
		@return {String}
		Get path part of the URL or empty string
		if there is no path in the URL.
		**/
		.set("path", &url::path)

		/**
		@function query()
		@return {String}
		Get query part of the URL or empty string
		if there is no query in the URL.
		**/
		.set("query", &url::query)

		/**
		@function fragment()
		@return {String}
		Get fragment part of the URL or empty string
		if there is no fragment in the URL.
		**/
		.set("fragment", &url::fragment)

		/**
		@function getWithEmptyPath()
		@return {URL}
		Return the URL without path, query and fragment parts
		**/
		.set("getWithEmptyPath", &url::authority)
		;

	http_module
		.set("Request", request_class)
		.set("Response", response_class)
		.set("Writer", writer_class)
		.set("Service", service_class)
		.set("FunctionService", function_service_class)
		.set("FilesystemService", filesystem_service_class)
//		.set("ExternalService", external_service_class)
		.set("WebsocketService", websocket_service_class)
		.set("Websocket", websocket_class)
		.set("Server", server_class)
		.set("Client", client_class)
		.set("Url", url_class)
		;

#define SET_HTTP_CONST(constant) http_module.set_const(#constant, pion::http::types::constant)
	/**
	@module http

	## Header name constants

	  * `HEADER_HOST`
	  * `HEADER_COOKIE`
	  * `HEADER_SET_COOKIE`
	  * `HEADER_CONNECTION`
	  * `HEADER_CONTENT_TYPE`
	  * `HEADER_CONTENT_LENGTH`
	  * `HEADER_CONTENT_LOCATION`
	  * `HEADER_CONTENT_ENCODING`
	  * `HEADER_LAST_MODIFIED`
	  * `HEADER_IF_MODIFIED_SINCE`
	  * `HEADER_TRANSFER_ENCODING`
	  * `HEADER_LOCATION`
	  * `HEADER_AUTHORIZATION`
	  * `HEADER_REFERER`
	  * `HEADER_USER_AGENT`
	  * `HEADER_X_FORWARDED_FOR`
	  * `HEADER_CLIENT_IP`
	**/
	SET_HTTP_CONST(HEADER_HOST);
	SET_HTTP_CONST(HEADER_COOKIE);
	SET_HTTP_CONST(HEADER_SET_COOKIE);
	SET_HTTP_CONST(HEADER_CONNECTION);
	SET_HTTP_CONST(HEADER_CONTENT_TYPE);
	SET_HTTP_CONST(HEADER_CONTENT_LENGTH);
	SET_HTTP_CONST(HEADER_CONTENT_LOCATION);
	SET_HTTP_CONST(HEADER_CONTENT_ENCODING);
	SET_HTTP_CONST(HEADER_LAST_MODIFIED);
	SET_HTTP_CONST(HEADER_IF_MODIFIED_SINCE);
	SET_HTTP_CONST(HEADER_TRANSFER_ENCODING);
	SET_HTTP_CONST(HEADER_LOCATION);
	SET_HTTP_CONST(HEADER_AUTHORIZATION);
	SET_HTTP_CONST(HEADER_REFERER);
	SET_HTTP_CONST(HEADER_USER_AGENT);
	SET_HTTP_CONST(HEADER_X_FORWARDED_FOR);
	SET_HTTP_CONST(HEADER_CLIENT_IP);

	/**
	@module http

	## Content type constants

	  * `CONTENT_TYPE_HTML`
	  * `CONTENT_TYPE_TEXT`
	  * `CONTENT_TYPE_XML`
	  * `CONTENT_TYPE_URLENCODED`
	  * `CONTENT_TYPE_JSON`
	**/
	SET_HTTP_CONST(CONTENT_TYPE_HTML);
	SET_HTTP_CONST(CONTENT_TYPE_TEXT);
	SET_HTTP_CONST(CONTENT_TYPE_XML);
	SET_HTTP_CONST(CONTENT_TYPE_URLENCODED);
	http_module.set_const("CONTENT_TYPE_JSON","application/javascript");

	/**
	@module http

	## Request method constants

	  * `REQUEST_METHOD_HEAD`
	  * `REQUEST_METHOD_GET`
	  * `REQUEST_METHOD_PUT`
	  * `REQUEST_METHOD_POST`
	  * `REQUEST_METHOD_DELETE`
	**/
	SET_HTTP_CONST(REQUEST_METHOD_HEAD);
	SET_HTTP_CONST(REQUEST_METHOD_GET);
	SET_HTTP_CONST(REQUEST_METHOD_PUT);
	SET_HTTP_CONST(REQUEST_METHOD_POST);
	SET_HTTP_CONST(REQUEST_METHOD_DELETE);

	/**
	@module http

	## Response message constants

	  * `RESPONSE_MESSAGE_OK`
	  * `RESPONSE_MESSAGE_CREATED`
	  * `RESPONSE_MESSAGE_NO_CONTENT`
	  * `RESPONSE_MESSAGE_FOUND`
	  * `RESPONSE_MESSAGE_UNAUTHORIZED`
	  * `RESPONSE_MESSAGE_FORBIDDEN`
	  * `RESPONSE_MESSAGE_NOT_FOUND`
	  * `RESPONSE_MESSAGE_METHOD_NOT_ALLOWED`
	  * `RESPONSE_MESSAGE_NOT_MODIFIED`
	  * `RESPONSE_MESSAGE_BAD_REQUEST`
	  * `RESPONSE_MESSAGE_SERVER_ERROR`
	  * `RESPONSE_MESSAGE_NOT_IMPLEMENTED`
	  * `RESPONSE_MESSAGE_CONTINUE`
	**/
	SET_HTTP_CONST(RESPONSE_MESSAGE_OK);
	SET_HTTP_CONST(RESPONSE_MESSAGE_CREATED);
	SET_HTTP_CONST(RESPONSE_MESSAGE_NO_CONTENT);
	SET_HTTP_CONST(RESPONSE_MESSAGE_FOUND);
	SET_HTTP_CONST(RESPONSE_MESSAGE_UNAUTHORIZED);
	SET_HTTP_CONST(RESPONSE_MESSAGE_FORBIDDEN);
	SET_HTTP_CONST(RESPONSE_MESSAGE_NOT_FOUND);
	SET_HTTP_CONST(RESPONSE_MESSAGE_METHOD_NOT_ALLOWED);
	SET_HTTP_CONST(RESPONSE_MESSAGE_NOT_MODIFIED);
	SET_HTTP_CONST(RESPONSE_MESSAGE_BAD_REQUEST);
	SET_HTTP_CONST(RESPONSE_MESSAGE_SERVER_ERROR);
	SET_HTTP_CONST(RESPONSE_MESSAGE_NOT_IMPLEMENTED);
	SET_HTTP_CONST(RESPONSE_MESSAGE_CONTINUE);

	/**
	@module http

	## Response code constants

	  * `RESPONSE_CODE_OK`
	  * `RESPONSE_CODE_CREATED`
	  * `RESPONSE_CODE_NO_CONTENT`
	  * `RESPONSE_CODE_FOUND`
	  * `RESPONSE_CODE_UNAUTHORIZED`
	  * `RESPONSE_CODE_FORBIDDEN`
	  * `RESPONSE_CODE_NOT_FOUND`
	  * `RESPONSE_CODE_METHOD_NOT_ALLOWED`
	  * `RESPONSE_CODE_NOT_MODIFIED`
	  * `RESPONSE_CODE_BAD_REQUEST`
	  * `RESPONSE_CODE_SERVER_ERROR`
	  * `RESPONSE_CODE_NOT_IMPLEMENTED`
	  * `RESPONSE_CODE_CONTINUE`
	**/
	SET_HTTP_CONST(RESPONSE_CODE_OK);
	SET_HTTP_CONST(RESPONSE_CODE_CREATED);
	SET_HTTP_CONST(RESPONSE_CODE_NO_CONTENT);
	SET_HTTP_CONST(RESPONSE_CODE_FOUND);
	SET_HTTP_CONST(RESPONSE_CODE_UNAUTHORIZED);
	SET_HTTP_CONST(RESPONSE_CODE_FORBIDDEN);
	SET_HTTP_CONST(RESPONSE_CODE_NOT_FOUND);
	SET_HTTP_CONST(RESPONSE_CODE_METHOD_NOT_ALLOWED);
	SET_HTTP_CONST(RESPONSE_CODE_NOT_MODIFIED);
	SET_HTTP_CONST(RESPONSE_CODE_BAD_REQUEST);
	SET_HTTP_CONST(RESPONSE_CODE_SERVER_ERROR);
	SET_HTTP_CONST(RESPONSE_CODE_NOT_IMPLEMENTED);
	SET_HTTP_CONST(RESPONSE_CODE_CONTINUE);
#undef SET_HTTP_CONST

	http_module.set_const("LOG_LEVEL_TRACE", 0);
	http_module.set_const("LOG_LEVEL_DEBUG", 1);
	http_module.set_const("LOG_LEVEL_INFO", 2);
	http_module.set_const("LOG_LEVEL_WARN", 3);
	http_module.set_const("LOG_LEVEL_ERROR", 4);
	http_module.set_const("LOG_LEVEL_PANIC", 5);


	// ~~~
#if HAVE(PION_LOGGER_DELEGATE)
	pion::logger_delegate* binding = new pion_logger_binding(runtime::instance(target.isolate()).get_logger()->get_multiplexor());
	boost::shared_ptr<pion::logger_delegate> lp(binding);
	pion::logger::generic().set_delegate(lp);
#endif

	http_module.set("set_log_level", set_log_level);

	/**
	@function makeQueryString(obj)
	@param obj {Object}
	@return {String}
	Create a query string from obj dictionary.
	Helper funciton to create HTTP queries from JavaScript objects:

	    var url = 'http://api.example.com/path?' + http.makeQueryString({ a : 1, b : 'text' });
	    console.log(url) // 'http://api.example.com/path?a=1&b=text'
	**/
	http_module.set("makeQueryString",  make_query_string);

	/**
	@function resolveAbsoluteLocation(target)
	@param target {String}
	@return {String}
	Attempts to locate a reference in a local script path and then in rte path.
	Returns absolute path to the located directory or file.
	This function is meant to be used when supplying relative paths to #FilesystemService
	**/
	http_module.set("resolveAbsoluteLocation", resolveAbsoluteLocation);

	target.set("http", http_module.new_instance());
}

void set_log_level(int level)
{
#if HAVE(PION_LOGGER_DELEGATE)
	pion::logger::generic().set_level(static_cast<pion::log_priority>(level));
#endif
}

void resolveAbsoluteLocation(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::string target = v8pp::from_v8<std::string>(isolate, args[0]);

	if (target.empty() || target[0] != '/')
	{
		target = '/' + target;
	}

	boost::filesystem::path p;
	runtime const& rt = runtime::instance(isolate);
	p = rt.script().parent_path() / target;
	if (!boost::filesystem::exists(p))
	{
		p = rt.rte_path() / target;
		if (!boost::filesystem::exists(p))
		{
			p = target;
		}
	}
	args.GetReturnValue().Set(v8pp::to_v8(isolate, p.normalize().native()));
}


v8::Handle<v8::Object> dict_to_object(v8::Isolate* isolate, pion::ihash_multimap const& dict)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> object = v8::Object::New(isolate);
	for (pion::ihash_multimap::const_iterator it = dict.begin(), end = dict.end(); it != end; ++it)
	{
		object->Set(v8pp::to_v8(isolate, it->first), v8pp::to_v8(isolate, pion::algorithm::url_decode(it->second)));
	}
	return scope.Escape(object);
}

pion::ihash_multimap object_to_dict(v8::Isolate* isolate, v8::Handle<v8::Object> object)
{
	v8::HandleScope scope(isolate);

	pion::ihash_multimap dict;
	v8::Local<v8::Array> prop_names = object->GetPropertyNames();
	for (uint32_t i = 0, count = prop_names->Length(); i != count; ++i)
	{
		std::string const name = v8pp::from_v8<std::string>(isolate, prop_names->Get(i));
		std::string const value = v8pp::from_v8<std::string>(isolate, object->Get(prop_names->Get(i)));
		dict.insert(std::make_pair(name, pion::algorithm::url_encode(value)));
	}
	return dict;
}

}} // aspect::http
