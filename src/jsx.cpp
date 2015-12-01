//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#include <boost/program_options.hpp>

#include "jsx/cpus.hpp"
#include "jsx/os.hpp"
#include "jsx/runtime.hpp"

namespace po = boost::program_options;

void print_version()
{
	std::cout << "HARMONY JSX Run-Time Engine " << CURRENT_CPU_STRING << " v" << HARMONY_RTE_VERSION << '\n';
	std::cout << "Copyright (c) 2011-2014 ASPECTRON Inc." << '\n';
	std::cout << "All Rights Reserved." << '\n';
}

void print_help(char const* program_name, po::options_description const& allowed_options)
{
	print_version();

	std::cout << "\n---------------------------------\n\n";
	std::cout << "Usage: " << program_name << " [options] script [script arguments]\n\n";
	std::cout << allowed_options << '\n';
}

// Program options extra style parser that captures as positional argument
// any element starting from the first positional argument.
// Implementation is similar to po::detail::cmdline::parse_terminator.
std::vector<po::option> script_args_parser(std::vector<std::string>& cmd_line_args)
{
	std::vector<po::option> result;

	std::string const& first_arg = cmd_line_args.front();
	if (!first_arg.empty() && first_arg[0] != '-')
	{
		std::transform(cmd_line_args.begin(), cmd_line_args.end(), std::back_inserter(result),
			[](std::string const& arg) -> po::option
			{
				po::option opt;
				opt.value.push_back(arg);
				opt.original_tokens.push_back(arg);
				opt.position_key = INT_MAX;
				return opt;
			});
		cmd_line_args.clear();
	}
	return result;
}

int main(int argc, char* argv[])
try
{
#if ENABLE(MSVC_MEMORY_LEAK_DETECTION) && OS(WINDOWS) && TARGET(DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

#if OS(WINDOWS) && ENABLE(WIN32_TOP_LEVEL_EXCEPTION_HANDLER)
	aspect::os::set_process_exception_handler();
#endif

	char const* const program_name = argv[0];

	using namespace aspect;
	using boost::filesystem::path;

	runtime::options options;
	path script;
	runtime::arguments script_args;

	bool is_silent = false;
	unsigned io_threads = 0;
	unsigned queue_threads = 0;

	// Declare allowed program options
	po::options_description generic_options("Generic options");
	generic_options.add_options()
		("help,h", "print help message and exit")
		("version,v", "print program version and exit")
		("v8-options", "print V8 command-line options and exit")
		;

	po::options_description runtime_options("Runtime options");
	runtime_options.add_options()
		("rte,r", po::value<path>(&options.rte)->default_value(options.rte), "run under specified rte folder name")
		("uuid,u", po::value<std::string>(&options.uuid), "set process uuid")
		("silent,t", po::bool_switch(&is_silent), "suppress console output")
		("io-threads", po::value<unsigned>(&io_threads)->default_value(cpu_count()), "number of I/O threads (0 - use number of CPU cores)")
		("queue-threads", po::value<unsigned>(&queue_threads)->default_value(0), "number of queue threads (0 - use default)")
		;

	po::options_description debug_options("Debug options");
	debug_options.add_options()
		("debug,d", po::bool_switch(&options.debug.enabled), "enable debugger support")
		("wait,w", po::bool_switch(&options.debug.wait), "wait for debugger connection")
		("port,p", po::value<uint16_t>(&options.debug.port)->default_value(options.debug.port), "set debugger port")
		;

	po::options_description allowed_options("allowed options are");
	allowed_options.add(generic_options).add(runtime_options).add(debug_options);

	po::options_description hidden_options("hidden options");
	hidden_options.add_options()
		("script,s", po::value<path>(&script), "run specified script")
		("args,a", po::value<runtime::arguments>(&script_args), "set script arguments")
		;

	po::positional_options_description positional_options;
	positional_options.add("args", -1); // option without name is a script option

	po::options_description cmd_line_options;
	cmd_line_options.add(allowed_options).add(hidden_options);

	po::command_line_parser cmd_line_parser(argc, argv);
	cmd_line_parser.options(cmd_line_options)
		.positional(positional_options).extra_style_parser(script_args_parser).allow_unregistered();
	po::parsed_options const parsed_opts = cmd_line_parser.run();

	po::variables_map vm;
	po::store(parsed_opts, vm);
	po::notify(vm);

	// handle command line
	if ( vm.count("help") )
	{
		print_help(program_name, allowed_options);
		return EXIT_SUCCESS;
	}
	if ( vm.count("version") )
	{
		print_version();
		return EXIT_SUCCESS;
	}
	if ( vm.count("v8-options") )
	{
		// V8 flag `help` will cause exit(), no reason to pass other flags
		argv[1] = "--help";
		runtime rt(2, argv);
		return EXIT_SUCCESS;
	}

	if ( script.empty() && !script_args.empty() )
	{
		// there is no `script` option, use first
		// positional argument as a script name.
		script = script_args.front();
		script_args.erase(script_args.begin());
	}

	if (script.empty())
	{
#if ENABLE(STANDALONE_SCRIPT_EXECUTION)
		print_help(program_name, allowed_options);
		return EXIT_FAILURE;
#else
		runtime::set_execution_options(runtime::get_execution_options() | runtime::EVENT_QUEUE);
		script = rte_path / "modules" / "module.main.js";
#endif
	}

	// collect unrecognized command line options for runtime and make new argv[argc]
	runtime::arguments const unregistered_args = po::collect_unrecognized(parsed_opts.options, po::exclude_positional);

	argc = static_cast<int>(unregistered_args.size() + 1);
	std::transform(unregistered_args.begin(), unregistered_args.end(), &argv[1],
		[](std::string const& arg) { return const_cast<char*>(arg.c_str()); });

	runtime rt(argc, argv, io_threads, queue_threads);

	if (is_silent)
	{
		rt.set_execution_options(rt.execution_options() | runtime::SILENT);
	}

	int const exit_code = rt.run(script, script_args, options);

#if ENABLE(MSVC_MEMORY_LEAK_DETECTION) && OS(WINDOWS) && TARGET(DEBUG)
	_CrtDumpMemoryLeaks();
#endif

	return exit_code;
}
catch (boost::program_options::error const& po_err)
{
	std::cerr << "Program options error: " << po_err.what() << '\n';
}
catch (std::exception const& ex)
{
	std::cerr << "Error: " << ex.what() << '\n';
}
catch (...)
{
	std::cerr << "Unknown exception\n";
}
