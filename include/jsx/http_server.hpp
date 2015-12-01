//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_HTTP_SERVER_HPP_INCLUDED
#define JSX_HTTP_SERVER_HPP_INCLUDED

#include "jsx/http.hpp"
#include "jsx/events.hpp"

namespace aspect {

class runtime;

namespace http {

class request
{
public:
	explicit request(pion::http::request_ptr impl)
		: impl_(impl)
	{
	}

private:
	pion::http::request_ptr impl_;
};

class response
{
public:
	explicit response(pion::http::request_ptr request)
		: impl_(new pion::http::response(*request))
	{
	}

	void set_content_type(std::string const& type) { impl_->set_content_type(type); }
	void set_status_code(uint32_t status_code) { impl_->set_status_code(status_code); }
	void set_status_message(const std::string& msg) { impl_->set_status_message(msg); }

	void set_header(std::string const& name, std::string const& value)
	{
		impl_->change_header(name, value);
	}

	void delete_header(std::string const& name)
	{
		impl_->delete_header(name);
	}

	void set_cookie(const std::string& name, const std::string& value, const std::string& path)
	{
		impl_->set_cookie(name, value, path);
	}

	void delete_cookie(const std::string& name, const std::string& path)
	{
		impl_->delete_cookie(name, path);
	}

	void set_last_modified(uint32_t t) { impl_->set_last_modified(t); }

private:
	friend class writer; // for impl_ access
	pion::http::response_ptr impl_;
};

class writer
{
public:
	writer(v8::Isolate* isolate, pion::http::request_ptr req, pion::tcp::connection_ptr conn);

	void write(v8::FunctionCallbackInfo<v8::Value> const& args);
	void send();
	void close();

	response* resp() { return resp_.get(); }

private:
	pion::http::response_writer_ptr impl_;
	pion::tcp::connection_ptr conn_;
	v8pp::persistent_ptr<response> resp_;
	bool is_conn_closed_;
	bool is_data_sent_;
};

/// HTTP service base class
class service : boost::noncopyable
{
public:
	service()
		: impl_(nullptr)
	{
	}

	virtual ~service() {}

	/// Pion plugin_service implementation
	pion::http::plugin_service* impl() const { return impl_; }

protected:
	pion::http::plugin_service* impl_;
	v8::UniquePersistent<v8::Value> obj_;
};

/// JS function handler web service
class function_service : public service
{
public:
	/// Create web service for the JS function
	function_service(v8::Isolate* isolate, v8::Handle<v8::Function> func);

private:
	runtime& rt_;

	class impl : public pion::http::plugin_service
	{
	public:
		explicit impl(function_service* service)
			: service_(service)
		{
		}

	private:
		function_service* service_;

		/// Webservice call
		virtual void operator()(pion::http::request_ptr& request, pion::tcp::connection_ptr& conn);

		void process(pion::http::request_ptr request, pion::tcp::connection_ptr conn);
	};
};

/// Filesystem webservice
class filesystem_service : public service
{
public:
	/// Create filesystem service for root_path
	explicit filesystem_service(v8::FunctionCallbackInfo<v8::Value> const& args);

	void clear_cache();
	void set_option(std::string const& name, std::string const& value);
};

/// External service
class external_service : public service
{
public:
	/// External service delegate
	class delegate
	{
	public:
		virtual ~delegate() {}

		virtual void digest(pion::http::request_ptr& request, pion::tcp::connection_ptr conn) = 0;
	};

	external_service()
	{
		impl_ = new impl;
	}

	void set_delegate(delegate* ptr)
	{
		static_cast<impl*>(impl_)->delegate_ = ptr;
	}

private:
	struct impl : pion::http::plugin_service
	{
		delegate* delegate_;

		impl() : delegate_(nullptr) {}

		virtual void operator()(pion::http::request_ptr& request, pion::tcp::connection_ptr& conn);
	};
};

/// Websocket service
class websocket_service : public service
{
public:
	websocket_service(v8::FunctionCallbackInfo<v8::Value> const& args);

	void on(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	struct impl
		: public pion::http::plugin_service
		, public v8_core::event_emitter
	{
		runtime& rt;

		explicit impl(runtime& rt) : rt(rt) {}
		virtual void operator()(pion::http::request_ptr& request, pion::tcp::connection_ptr& conn);
	};
};

/// HTTP server
class server
{
	typedef pion::http::plugin_server base_class;
public:
	/// Create new HTTP server
	explicit server(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Stop server on destroy
	~server();

	/// Start server (declared here for v8 accessibility)
	void start(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Stop server (declared here for v8 accessibility)
	void stop(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Set Auth
	void set_auth(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Set SSL keyfile (customized relay function for v8)
	void set_ssl_keyfile(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Set resource handler (either function or web service)
	void digest(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Clear server cache
	void clear_cache(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	/// Pion scheduler for http_server, returns runtime io_service
	struct scheduler : public pion::scheduler
	{
		explicit scheduler(runtime& rt);
		~scheduler();
		virtual boost::asio::io_service& get_io_service();

		runtime& rt;
	};

	scheduler scheduler_;
	pion::http::plugin_server impl_;
	typedef std::vector<v8::UniquePersistent<v8::Value>> services;
	services services_;
};

}} // namespace aspect::http

#endif // JSX_HTTP_SERVER_HPP_INCLUDED
