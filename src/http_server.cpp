//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#include "jsx/core.hpp"
#include "jsx/http_server.hpp"

#include "jsx/async_queue.hpp"
#include "jsx/runtime.hpp"
#include "jsx/v8_buffer.hpp"
#include "jsx/v8_main_loop.hpp"

#include "jsx/websocket.hpp"

#include <pion/http/basic_auth.hpp>

#include "FileService.hpp"

namespace aspect { namespace http {

#if 0
uint64_t last_ts = 0;
uint32_t req_acc = 0;
uint32_t req = 0;
void __test_access_performance__(pion::http::request_ptr& request, pion::tcp::connection_ptr tcp_conn)
{
	req_acc++;
	uint64_t ts = utils::get_ts64();
	if(ts - last_ts > 1000)
	{
		last_ts = ts;
		req = req_acc;
		req_acc = 0;
	}

	char buffer[1024];
	sprintf_s(buffer,sizeof(buffer),"Requests/Sec: %d<p/>",req);

	std::string str;
	str += "<html><head><meta http-equiv=\'refresh\' content=\'100;\'/></head><body> hello web browser world! - NATIVE <p/>";
	str += "Requests / Sec: ";
	str += buffer;
	str += "</body></html>";

	pion::net::HTTPResponseWriterPtr writer = pion::net::HTTPResponseWriter::create(tcp_conn, *request, boost::bind(&pion::tcp::connection::finish, tcp_conn));
	writer->write(str);
	writer->send();
	return;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// http_writer
//
writer::writer(v8::Isolate* isolate, pion::http::request_ptr req, pion::tcp::connection_ptr conn)
	: conn_(conn)
	, is_conn_closed_(false)
	, is_data_sent_(false)
{
	response* resp = new response(req);
	v8pp::class_<response>::import_external(isolate, resp);
	resp_.reset(isolate, resp);

	impl_ = pion::http::response_writer::create(conn_, resp_->impl_,
		boost::bind(&pion::tcp::connection::finish, conn_));
}

void writer::write(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);
	v8::Local<v8::Value> value = args[0];

	if (value->IsString() )
	{
		v8::String::Utf8Value const str(value);
		impl_->write(*str, str.length());
	}
	else if (v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(isolate, value))
	{
		impl_->write(buf->data(), buf->size());
	}
	else
	{
		throw std::invalid_argument("expecting string or native buffer object");
	}
}

void writer::send()
{
	if (!is_data_sent_ && !is_conn_closed_)
	{
		impl_->send();
		is_data_sent_ = true;
	}
}

void writer::close()
{
	if (!is_conn_closed_)
	{
		impl_->get_connection()->close();
		is_conn_closed_ = true;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// function_service
//
function_service::function_service(v8::Isolate* isolate, v8::Handle<v8::Function> func)
	: rt_(runtime::instance(isolate))
{
	impl_ = new impl(this);
	obj_.Reset(isolate, func);
}


void function_service::impl::operator()(pion::http::request_ptr& request, pion::tcp::connection_ptr& tcp_conn)
{
#if 0
	__test_access_performance__(request, tcp_conn);
	return;
#endif
	service_->rt_.main_loop().schedule(std::bind(&function_service::impl::process, this, request, tcp_conn));
}

void function_service::impl::process(pion::http::request_ptr req, pion::tcp::connection_ptr tcp_conn)
try
{
	v8::Isolate* isolate = service_->rt_.isolate();

	// NOTE - we disable NGALE algorithm
	tcp_conn->get_socket().set_option(boost::asio::ip::tcp::no_delay(true));

	v8::HandleScope scope(isolate);

	// writer object
	writer* w = new writer(isolate, req, tcp_conn);
	v8::Local<v8::Object> writer_object = v8pp::class_<writer>::import_external(isolate, w);

	// -- request assembly
	v8::Local<v8::Object> request_object = v8::Object::New(isolate);
	v8::Local<v8::Object> js_req = v8pp::class_<request>::import_external(isolate, new request(req));

	// Update actual JavaScript documentation for the request object in http_install()
	aspect::set_option(isolate, request_object, "interface", js_req);
	aspect::set_option(isolate, request_object, "version", req->get_version_string());
	aspect::set_option(isolate, request_object, "method", req->get_method());
	aspect::set_option(isolate, request_object, "resource", pion::algorithm::url_decode(req->get_resource()));
	aspect::set_option(isolate, request_object, "queryString", pion::algorithm::url_decode(req->get_query_string()));
	aspect::set_option(isolate, request_object, "query", dict_to_object(isolate, req->get_queries()));
	aspect::set_option(isolate, request_object, "headers", dict_to_object(isolate, req->get_headers()));
	aspect::set_option(isolate, request_object, "cookies", dict_to_object(isolate, req->get_cookies()));
	aspect::set_option(isolate, request_object, "ip", req->get_remote_ip().to_string());
	if (req->get_content_length())
	{
		v8_core::buffer* content = new v8_core::buffer(req->get_content(), req->get_content_length());
		aspect::set_option(isolate, request_object, "content", v8pp::class_<v8_core::buffer>::import_external(isolate, content));
	}

	// --

	v8::Local<v8::Function> function = v8pp::to_local(isolate, service_->obj_).As<v8::Function>();
	v8::Local<v8::Object> self = v8pp::to_v8(isolate, service_)->ToObject();
	v8::Handle<v8::Value> argv[] = { request_object, writer_object };

	v8::TryCatch try_catch;
	function->Call(self, sizeof(argv) / sizeof(argv[0]), argv);
	if ( try_catch.HasCaught() )
	{
		service_->rt_.core().report_exception(try_catch);
	}
	w->send();
}
catch (std::exception const& ex)
{
	service_->rt_.error(ex.what());
}

/////////////////////////////////////////////////////////////////////////////
//
// filesystem_service
//
filesystem_service::filesystem_service(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::string const root_path = v8pp::from_v8<std::string>(args.GetIsolate(), args[0]);

	std::unique_ptr<pion::plugins::FileService> fs(new pion::plugins::FileService);

	runtime& rt = runtime::instance(args.GetIsolate());

	using namespace boost::filesystem;
	
	path p = root_path;
	p.make_preferred();

	if ( !exists(p) )
	{
		p = rt.rte_path() / p;
	}
	if ( !exists(p) )
	{
		rt.error("http service error: unable to locate root path %s\n", root_path.c_str());
		throw std::runtime_error("unable to locate root path " + root_path);
	}

	if ( is_regular_file(p) )
	{
		fs->set_option("file", p.string());
		p = p.parent_path();
	}

	if ( !is_directory(p) )
	{
		rt.error("http service error: unable to locate root path %s\n", p.c_str());
		throw std::runtime_error("unable to locate root path " + p.string());
	}

	// rt.trace("HTTP - Setting filesystem_service root path to: %s", path.string().c_str());
	fs->set_option("directory", p.string());

	impl_ = fs.release();
}

void filesystem_service::clear_cache()
{
	static_cast<pion::plugins::FileService*>(impl())->clear_cache();
}

void filesystem_service::set_option(std::string const& name, std::string const& value)
{
	static_cast<pion::plugins::FileService*>(impl())->set_option(name, value);
}

//////////////////////////////////////////////////////////////////////////
//
// external_service
//
void external_service::impl::operator()(pion::http::request_ptr& req, pion::tcp::connection_ptr& tcp_conn)
{
	if (req->is_valid() && delegate_)
	{
		delegate_->digest(req, tcp_conn);
	}
	else if (!delegate_)
	{
		tcp_conn->close();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// websocket_service
//
websocket_service::websocket_service(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	impl_ = new impl(runtime::instance(args.GetIsolate()));
}

void websocket_service::on(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::String> str = args[0]->ToString();
	v8::Local<v8::Function> callback = args[1].As<v8::Function>();

	if (str.IsEmpty() || callback.IsEmpty())
	{
		throw std::runtime_error("require event name string and listener function arguments");
	}

	v8::String::Utf8Value const name(str);
	static_cast<impl*>(impl_)->on(isolate, *name, callback);

	args.GetReturnValue().Set(args.This());
}

void websocket_service::impl::operator()(pion::http::request_ptr& req, pion::tcp::connection_ptr& tcp_conn)
{
	if ( req->is_valid() && websocket::is_websocket_upgrade(*req) )
	{
		rt.main_loop().schedule(std::bind(&websocket::server_start, rt.isolate(), req, tcp_conn, this));
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// server
//
server::scheduler::scheduler(runtime& rt)
	: rt(rt)
{
	set_num_threads(static_cast<uint32_t>(rt.io_pool().num_threads()));
	startup();
}

server::scheduler::~scheduler()
{
	shutdown();
}

boost::asio::io_service& server::scheduler::get_io_service()
{
	return rt.io_pool().io_service();
}

server::server(v8::FunctionCallbackInfo<v8::Value> const& args)
	: scheduler_(runtime::instance(args.GetIsolate()))
	, impl_(scheduler_, v8pp::from_v8<unsigned>(args.GetIsolate(), args[0]))
{
}

server::~server()
{
	impl_.stop();
	services_.clear();
}

void server::start(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	impl_.start();

	args.GetReturnValue().Set(args.This());
}

void server::stop(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	bool const wait_until_finish = v8pp::from_v8<bool>(args.GetIsolate(), args[0], false);

	impl_.stop(wait_until_finish);

	args.GetReturnValue().Set(args.This());
}

void server::set_ssl_keyfile(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	std::string const filename = v8pp::from_v8<std::string>(args.GetIsolate(), args[0]);

	impl_.set_ssl_key_file(filename);

	args.GetReturnValue().Set(args.This());
}

void server::set_auth(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> obj = args[0]->ToObject();
	if (obj.IsEmpty() || !obj->IsObject())
	{
		throw std::invalid_argument("http_server::set_auth() expects object as argument");
	}

	std::string realm = "Please login";
	get_option(isolate, obj, "realm", realm);

	pion::user_manager_ptr user_mgr(new pion::user_manager);
	pion::http::auth_ptr auth_ptr(new pion::http::basic_auth(user_mgr, realm));

	v8::Local<v8::Object> restrict;
	get_option(isolate, obj, "restrict", restrict);
	if (!restrict.IsEmpty() && !restrict->IsUndefined())
	{
		v8::Local<v8::Array> props = restrict->GetPropertyNames();
		for (uint32_t i = 0, count = props->Length(); i != count; ++i)
		{
			v8::Handle<v8::String> resource = props->Get(i)->ToString();
			if ( resource.IsEmpty() )
			{
				throw std::runtime_error("Unable to digest auth; Object contains invalid path (not a string)");
			}

			// add restrict
			auth_ptr->add_restrict(v8pp::from_v8<std::string>(isolate, resource));

			v8::Local<v8::Object> users = restrict->Get(resource)->ToObject();
			if ( users.IsEmpty() )
			{
				throw std::runtime_error(std::string("Unable to digest auth; Invalid user object."));
			}

			v8::Local<v8::Array> usernames = users->GetPropertyNames();
			for (uint32_t n = 0, n_count = usernames->Length(); n != n_count; ++n)
			{
				v8::Local<v8::String> user = usernames->Get(n)->ToString();
				v8::Local<v8::String> pass = users->Get(user)->ToString();

				auth_ptr->add_user(v8pp::from_v8<std::string>(isolate, user), v8pp::from_v8<std::string>(isolate, pass));
			}
		}
	}

	v8::Local<v8::Array> allow;
	get_option(isolate, obj, "allow", allow);
	if (!allow.IsEmpty() && !allow->IsUndefined())
	{
		for (uint32_t i = 0, count = allow->Length(); i != count ; ++i)
		{
			v8::Local<v8::String> str = allow->Get(i)->ToString();
			if ( str.IsEmpty() )
			{
				throw std::invalid_argument("http auth allow array elements must be strings");
			}
			auth_ptr->add_permit(v8pp::from_v8<std::string>(isolate, str));
		}
	}

	impl_.set_authentication(auth_ptr);

	args.GetReturnValue().Set(args.This());
}

void server::digest(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	std::string const resource = v8pp::from_v8<std::string>(isolate, args[0]);
	if ( resource.empty() )
	{
		throw std::invalid_argument("Error: invalid resource specified");
	}

	service* s = nullptr;

	v8::Local<v8::Value> js_service = args[1];
	if (js_service->IsFunction())
	{
		function_service* fs = new function_service(isolate, js_service.As<v8::Function>());
		js_service = v8pp::class_<function_service>::import_external(isolate, fs);
		s = fs;
	}
	else if (js_service->IsObject())
	{
		s = v8pp::from_v8<service*>(isolate, js_service);
		if (!s)
		{
			throw std::invalid_argument("digest() requires http_service or function as a second parameter (received unknown object)");
		}
	}
	else
	{
		throw std::invalid_argument("digest() requires http_service or function as a second parameter");
	}

	services_.emplace_back(v8::UniquePersistent<v8::Value>(isolate, js_service));
	impl_.add_service(resource, s->impl());

	args.GetReturnValue().Set(args.This());
}

void server::clear_cache(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::for_each(services_.begin(), services_.end(),
		[isolate](v8::UniquePersistent<v8::Value>& value)
		{
			service* s = v8pp::class_<service>::unwrap_object(isolate, v8pp::to_local(isolate, value));
			if (filesystem_service* fs = dynamic_cast<filesystem_service*>(s))
			{
				fs->clear_cache();
			}
		});

	args.GetReturnValue().Set(args.This());
}

}} // namespace aspect::http
