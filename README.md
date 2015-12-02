# JSX

JSX is a C++ library which allows to use JavaScript in a C++ application. It based on [V8 JavaScript engine](https://code.google.com/p/v8/) and supports [CommonJS](http://en.wikipedia.org/wiki/CommonJS) conformant modules.


## Prerequisites

Python 2.7+, Perl and appropriate build tools are required to build the project.

### Windows

Visual C++ 2010 or newer including Express editions.

To build x64 project with Visual C++ 2010 Express download and install the
[Windows Software Development Kit version 7.1](http://www.microsoft.com/en-us/download/details.aspx?id=8279).
Visual C++ 2010 Express does not include a 64 bit compiler, but the SDK does.
If you still doesn't have x64 compiler for Visual C++ 2010 please read this:
http://support.microsoft.com/kb/2519277


### Linux

Platform build tools (C++ compiler, make), Perl.

There is a boostrasph.sh script for Debian/Ubuntu systems, run it as administrator:

    sudo bootstrap.sh


### Mac OSX

Xcode as well as its command line tools.

You need to run xcodebuild -license to accept the license if the GUI has never been run.


## Building

HARMONY JSX has been built sucessfully on Windows, Linux and Mac OSX with [GYP](https://code.google.com/p/gyp/). There is a build.py script for project files generation and building.

To build the project default configuration please run:

    python build.py

Allowed build options would be printed on:

    python build.py --help


## Running

The project has a `jsx` application for running JavaScript code in the JSX environment.

On successful build all targets would be placed in the result directiry `bin/<build configuration>`. There are several sample files in a `rte/sandbox` directory. On Windows please run command like following in the project directory:

    "bin\Release x64\jsx.exe" rte\sandbox\globals.js

On Linux it would be similar to:

    bin/jsx rte/sandbox/globals.js

On Mac OSX:

    bin/Release/jsx rte/sandbox/globals.js

Additional help on the `jsx` application command-line options:

    bin/jsx --help


## Documentation

Documentation for JSX JavaScript API will be generated on the project build in a `doc/jsx` directory. Documentation for C++ API can be generated with Doxygen in a `doc` directory.


## Usage

Please refer to `src/jsx.cpp` as a JSX embedding example. There is a `aspect::runtime` class that represents a JSX instance. 
Create it and run a JavaScript file:

	#include "jsx/runtime.hpp"

	int main()
	{
		aspect::runtime rt;
		int const exit_code = rt.run("myscript.js");
		return exit_code;
	}

It's possible to have multiple JSX runtimes that are running in a different threads. To access a JSX runtime from a V8 invocation callback use `aspect::runtime::instance()` function to get the runtime for a `v8::Isolate` instance:

	#include <v8.h>

	#include "jsx/runtime.hpp"
	#include "jsx/v8_main_loop.hpp"

	void my_v8_function(v8::FunctionCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		aspect::runtime& rt = aspect::runtime::instance(isolate);

		rt.trace("terminating");
		rt.terminate(-1);
	}

Most of JSX functions and classes exposed to JavaScript API are also available from C++ code.

### Making custom extension module

JSX can be extended with a custom shared library. Export to JavaScript required functions and classes from the shared library:

	#include "jsx/library.hpp"

	class some_class
	{
	public:
		int mem_function(float arg1, int arg2) const;
		void other_mem_function();
	};

	void some_function();

	DECLARE_LIBRARY_ENTRYPOINTS(library_install, library_uninstall);

	v8::Handle<v8::Value> library_install(v8::Isolate* isolate)
	{
		v8pp::module module(isolate);

		v8pp::class_<some_class> js_some_class(isolate);
		js_some_class
			.set("method_name", &some_class::method)
			.set("method_name2", &some_class::other_mem_function);
		module.set("some_class", js_some_class);

		module.set("function_name", some_function);

		return test_module.new_instance();
	}

	void library_uninstall(v8::Isolate* isolate, v8::Handle<v8::Value> library)
	{
		(void)library;
		v8pp::class_<some_class>::destroy_objects(isolate);
	}

Create a `library.gyp` file for the library with the following structure:

	{
	    'targets': [
	        {
	            'target_name': 'library_name',
	            'type': 'shared_library',
	            'dependencies': [
	                '<(jsx)/jsx-lib.gyp:jsx-lib',
	                '<(jsx)/extern/extern.gyp:*',
	            ],
	            'sources': [ 'library.cpp', 'other_library.cpp' ],
	        },
	    ],
	}

Create an utilty Python `build.py` script to build the library with GYP:

	#!/usr/bin/env python

	import os
	import sys

	def main():
	    jsx_root = os.environ.get('JSX_ROOT', os.path.join(os.path.pardir, 'jsx'))
	    jsx_root = os.path.abspath(jsx_root)
	    jsx_extern = os.path.join(jsx_root, 'extern')
	    
	    try:
	        sys.path.insert(0, jsx_extern)
	        from build_utils import parse_args, build_project
	    except:
	        sys.exit('Error: JSX root not found in {}'.format(jsx_root))

	    args = parse_args()
	    if not args.gyp_file:
	        args.gyp_file = 'library.gyp'
	    if not args.jsx_root:
	        args.jsx_root = jsx_root

	    build_project(args)    


	if __name__ == '__main__':
	    main()

Set an environment variable `JSX_ROOT` to the JSX root directory and run the `build.py` script.
