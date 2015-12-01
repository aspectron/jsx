//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_GEOMETRY_HPP_INCLUDED
#define JSX_GEOMETRY_HPP_INCLUDED

#include <boost/operators.hpp>

#include "jsx/v8_core.hpp"

namespace aspect {

/// 2D point
template<typename T>
struct point : public boost::equality_comparable<point<T>>
{
	T x, y;

	/// Create zero point
	point()
		: x()
		, y()
	{
	}

	/// Create (x, y) point
	point(T x, T y)
		: x(x)
		, y(y)
	{
	}

	bool is_null() const
	{
		return x == 0 && y == 0;
	}

	bool operator==(point const& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}
};

/// 2D box
template<typename T>
struct box : public boost::equality_comparable<box<T>>
{
	T width, height;

	/// Create empty size
	box()
		: width()
		, height()
	{
	}

	/// Create a size
	box(T width, T height)
		: width(width)
		, height(height)
	{
	}

	bool is_empty() const
	{
		return width == 0 && height == 0;
	}

	bool operator==(box const& rhs) const
	{
		return width == rhs.width && height == rhs.height;
	}
};

/// 2D rectangle
template<typename T>
struct rectangle : public boost::equality_comparable<rectangle<T>>
{
	T left, top;
	T width, height;

	/// Create empty rectangle
	rectangle()
		: left()
		, top()
		, width()
		, height()
	{
	}

	/// Create a rectangle
	rectangle(T left, T top, T width, T height)
		: left(left)
		, top(top)
		, width(width)
		, height(height)
	{
	}

	/// Create a rectangle
	rectangle(point<T> const& left_top, box<T> const& width_height)
		: left(left_top.x)
		, top(left_top.y)
		, width(width_height.width)
		, height(width_height.height)
	{
	}

	T bottom() const { return top + height; }
	T right() const { return left + width; }

	bool is_empty() const
	{
		return width == 0 && height == 0;
	}

	bool is_null() const
	{
		return left == 0 && top == 0 && is_empty();
	}

	bool operator==(rectangle const& rhs) const
	{
		return left == rhs.left && top == rhs.top
			&& width == rhs.width && height == rhs.height;
	}

};

typedef point<int32_t> image_point;
typedef box<int32_t> image_size;
typedef rectangle<int32_t> image_rect;

// this tests if the rect is FULLY INSIDE of another rect
inline bool operator < (image_rect const& a, image_rect const& b)
{
	return a.left > b.left && a.top > b.top &&
		a.left+a.width < b.left+b.width &&
		a.top+a.height < b.top+b.height;
}

inline bool operator <= (image_rect const& a, image_rect const& b)
{
	return a.left >= b.left && a.top >= b.top &&
		a.left+a.width <= b.left+b.width &&
		a.top+a.height <= b.top+b.height;
}

// this tests if the rect is FULLY OUTSIDE of another rect
inline bool operator > (image_rect const& a, image_rect const& b)
{
	return a.left < b.left && a.top < b.top &&
		a.left+a.width > b.left+b.width &&
		a.top+a.height > b.top+b.height;
}

inline bool operator >= (image_rect const& a, image_rect const& b)
{
	return a.left <= b.left && a.top <= b.top &&
		a.left+a.width >= b.left+b.width &&
		a.top+a.height >= b.top+b.height;
}

} // namespace aspect {

namespace v8pp {

using aspect::get_option;
using aspect::set_option;

template<typename T>
struct convert< aspect::point<T> >
{
	typedef aspect::point<T> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		v8::HandleScope scope(isolate);

		result_type result;

		if (value->IsArray())
		{
			v8::Local<v8::Array> arr = value.As<v8::Array>();
			v8::Local<v8::Value> x = arr->Get(0), y = arr->Get(1);
			if (arr->Length() != 2 || !x->IsNumber() || !y->IsNumber())
			{
				throw std::invalid_argument("expected [x, y] array");
			}
			result.x = v8pp::from_v8<T>(isolate, x);
			result.y = v8pp::from_v8<T>(isolate, y);
		}
		else if (value->IsObject())
		{
			v8::Local<v8::Object> obj = value.As<v8::Object>();
			if (!get_option(isolate, obj, "x", result.x) || !get_option(isolate, obj, "y", result.y))
			{
				throw std::invalid_argument("expected {x, y} object");
			}
		}
		else
		{
			throw std::invalid_argument("expected [x, y] array or {x, y} object");
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, aspect::point<T> const& value)
	{
		v8::EscapableHandleScope scope(isolate);

		v8::Local<v8::Object> obj = v8::Object::New(isolate);
		set_option(isolate, obj, "x", value.x);
		set_option(isolate, obj, "y", value.y);

		return scope.Escape(obj);
	}
};

template<typename T>
struct convert< aspect::box<T> >
{
	typedef aspect::box<T> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		v8::HandleScope scope(isolate);

		result_type result;

		if (value->IsArray())
		{
			v8::Local<v8::Array> arr = value.As<v8::Array>();
			v8::Local<v8::Value> width = arr->Get(0), height = arr->Get(1);
			if (arr->Length() != 2 || !width->IsNumber() || !height->IsNumber())
			{
				throw std::invalid_argument("expected [width, height] array");
			}
			result.width = v8pp::from_v8<T>(isolate, width);
			result.height = v8pp::from_v8<T>(isolate, height);
		}
		else if (value->IsObject())
		{
			v8::Local<v8::Object> obj = value.As<v8::Object>();
			if (!get_option(isolate, obj, "width", result.width) || !get_option(isolate, obj, "height", result.height))
			{
				throw std::invalid_argument("expected {width, height} object");
			}
		}
		else
		{
			throw std::invalid_argument("expected [width, height] array or {width, height} object");
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, aspect::box<T> const& value)
	{
		v8::EscapableHandleScope scope(isolate);

		v8::Local<v8::Object> obj = v8::Object::New(isolate);
		set_option(isolate, obj, "width", value.width);
		set_option(isolate, obj, "height", value.height);

		return scope.Escape(obj);
	}
};

template<typename T>
struct convert< aspect::rectangle<T> >
{
	typedef aspect::rectangle<T> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		v8::HandleScope scope(isolate);

		result_type result;

		if (value->IsArray())
		{
			v8::Local<v8::Array> arr = value.As<v8::Array>();
			v8::Local<v8::Value> left = arr->Get(0), top = arr->Get(1);
			v8::Local<v8::Value> width = arr->Get(2), height = arr->Get(3);
			if (arr->Length() != 4 || !left->IsNumber() || !top->IsNumber()
				|| !width->IsNumber() || !height->IsNumber())
			{
				throw std::invalid_argument("expected [left, top, width, height] array");
			}
			result.left= v8pp::from_v8<T>(isolate, left);
			result.top = v8pp::from_v8<T>(isolate, top);
			result.width = v8pp::from_v8<T>(isolate, width);
			result.height = v8pp::from_v8<T>(isolate, height);
		}
		else if (value->IsObject())
		{
			v8::Local<v8::Object> obj = value.As<v8::Object>();
			if (!get_option(isolate, obj, "left", result.left)
				|| !get_option(isolate, obj, "top", result.top)
				|| !get_option(isolate, obj, "width", result.width)
				|| !get_option(isolate, obj, "height", result.height))
			{
				throw std::invalid_argument("expected {left, top, width, height} object");
			}
		}
		else
		{
			throw std::invalid_argument("expected [left, top, width, height] array or {left, top, width, height} object");
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, aspect::rectangle<T> const& value)
	{
		v8::EscapableHandleScope scope(isolate);

		v8::Local<v8::Object> obj = v8::Object::New(isolate);
		set_option(isolate, obj, "left", value.left);
		set_option(isolate, obj, "top", value.top);
		set_option(isolate, obj, "width", value.width);
		set_option(isolate, obj, "height", value.height);

		return scope.Escape(obj);
	}
};

template<typename T>
struct is_wrapped_class< aspect::point<T> > : boost::false_type {};

template<typename T>
struct is_wrapped_class< aspect::box<T> > : boost::false_type {};

template<typename T>
struct is_wrapped_class< aspect::rectangle<T> > : boost::false_type {};

} // v8pp

#endif // JSX_GEOMETRY_HPP_INCLUDED
