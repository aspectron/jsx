//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#pragma once
#ifndef __ALIGNED_ALLOCATOR_HPP__
#define __ALIGNED_ALLOCATOR_HPP__

#include <stdlib.h>

#include <memory>
#include <stdexcept>

template<class T, size_t alignment>
struct aligned_allocator : std::allocator<T>
{
	static_assert(alignment >= sizeof(void*) && (alignment & (alignment - 1)) == 0,
		"alignment must be a power of 2 at least as large as sizeof(void *)");

	template<class U>
	struct rebind { typedef aligned_allocator<U, alignment> other; };

	typedef std::allocator<T> base;

	typedef typename base::pointer pointer;
	typedef typename base::size_type size_type;

	struct bad_alloc : std::bad_alloc
	{
		const char* what() const throw() { return "aligned_allocator"; }
	};

	aligned_allocator()
	{}

	template<class U>
	aligned_allocator(U const&)
	{}

	pointer allocate(size_type n)
	{
		if(pointer p = this->aligned_malloc(n))
			return p;
		throw bad_alloc();
	}

	pointer allocate(size_type n, void const*)
	{
		return this->allocate(n);
	}

	void deallocate(pointer p, size_type)
	{
		this->aligned_free(p);
	}

private:
	pointer aligned_malloc(size_type n)
	{
		void* p;
#ifdef _MSC_VER
		p = _aligned_malloc(n, alignment);
#else
		if (posix_memalign(&p, alignment, n) != 0)
		{
			p = NULL;
		}
#endif
		return static_cast<pointer>(p);
	}

	void aligned_free(pointer p)
	{
#ifdef _MSC_VER
		 _aligned_free(p);
#else
		free(p);
#endif
	}
};

#endif // __ALIGNED_ALLOCATOR_HPP__
