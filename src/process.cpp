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
#include "jsx/process.hpp"

#include "jsx/utils.hpp"

#if OS(WINDOWS)
#include <psapi.h>
#include <winternl.h>
#else
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#endif

#if OS(DARWIN)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/sysctl.h>
#endif

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

//////////////////////////////////////////////////////////////////////////

namespace aspect { namespace process {

void start_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if ( args.Length() < 1 || !args[0]->IsString() )
	{
		throw std::runtime_error("requires command line string argument");
	}

	std::string const command_line = v8pp::from_v8<std::string>(isolate, args[0]);
	int priority = PRIORITY_NORMAL;
	if ( args.Length() > 1 && args[1]->IsNumber() )
	{
		priority = v8pp::from_v8<int>(isolate, args[1]);
	}
	pid_t const pid = start(command_line, priority);
	args.GetReturnValue().Set(v8pp::to_v8(isolate, pid));
}

void setup_bindings(v8pp::module& target)
{
	/**
	@module process Process
	Process management functions
	**/
	v8pp::module process_module(target.isolate());

#define SET_CONST(name) process_module.set_const(#name, name)
	SET_CONST(PRIORITY_IDLE);
	SET_CONST(PRIORITY_BELOW_NORMAL);
	SET_CONST(PRIORITY_NORMAL);
	SET_CONST(PRIORITY_ABOVE_NORMAL);
	SET_CONST(PRIORITY_HIGH);
	SET_CONST(PRIORITY_REALTIME);
#undef SET_CONST

	process_module
		/**
		@function start(cmdline [, priority])
		@param cmdline {String} Command-line
		@param [priority=PRIORITY_NORMAL] {Number} Process priority
		@return {Number} process ID
		Start a new process with command line string.
		An optional priority value could be used to set the process priority.
		See also #setPriority
		**/
		.set("start", start_v8)

		/**
		@function kill(pid)
		@param pid {Number} process ID
		Kill a process with specified ID
		**/
		.set("kill", kill)

		/**
		@function current()
		@return {Number} current process ID
		Return current process ID
		**/
		.set("current", current)

		/**
		@function isRunning(pid)
		@param pid {Number} process ID
		@return {Boolean}
		Check is a process with specified `pid` running
		**/
		.set("isRunning", is_running)

		/**
		@function list()
		@return {Array}
		Get a list of currently running process identifiers.
		**/
		.set("list", list)

		/**
		@function getInfo(pid)
		@param pid {Number} process ID
		@return {Object}
		Get process information for specified process ID.

		Process information object consists of the following properties:

		  * `isRunning`  Wether the process running or not
		  * `name`       Process name string
		  * `cmdLine`    Process command line string

		In Windows there are additional properties in the information object:

		  * `memory`            Memory usage:
		    * `workingSetSize`    current working set size in bytes
		    * `pagefileUsage`     committed virtual memory in bytes

		  * `cpu`     CPU usage, the same as #getCpuUsage returns:
		    * `start`   Process start time
		    * `exit`    Process exit time , 0 if the process is still running
		    * `kernel`  Kernel mode execution time
		    * `user`    User mode excution time
		    * `total`   Total process excution time

		Process start and exit time values are number of milliseconds since
		the Unix Epoch: Jan 1st, 1970. This allows to construct a Date objects:

		    var currInfo = process.getInfo(process.current());
		    var startTime = new Date(currInfo.cpu.start);
		    console.log('current process started at', startTime);

		**/
		.set("getInfo", get_info)

		/**
		@function setPriority(pid, priority)
		@param pid {Number} process ID
		@param priority
		Set a process priority. Priority could be one of the following values:
		  * `PRIORITY_IDLE`
		  * `PRIORITY_BELOW_NORMAL`
		  * `PRIORITY_NORMAL`
		  * `PRIORITY_ABOVE_NORMAL`
		  * `PRIORITY_HIGH`
		  * `PRIORITY_REALTIME`
		**/
		.set("setPriority", set_priority)

		/**
		@function getCpuUsage([pid])
		@param [pid] {Number} process ID
		@return {Object}
		Get CPU usage information.
		Return an object with CPU usage information for specified proccess `pid`
		with properties:
		  * `start`   Start time from Unix epoch in milliseconds
		  * `exit`    Exit time from Unix epoch in milliseconds, 0 if the process is running
		  * `kernel`  Kernel mode execution time, milliseconds
		  * `user`    User mode excution time, milliseconds
		  * `total`   Total process excution time, milliseconds

		If process ID is not specified, the system CPU usage object would be returned:
		  * `idle`    System execution time in idle mode, milliseconds
		  * `kernel`  System execution time in kernel mode, milliseconds
		  * `user`    System execution time in user mode, milliseconds
		  * `total`   Total system execution time, milliseconds
		**/
		.set("getCpuUsage", get_cpu_usage)

		.set("signalNamedCondition", signal_named_condition)
		;

	target.set("process", process_module.new_instance());
}

#if OS(WINDOWS)

BOOST_STATIC_ASSERT(sizeof(pid_t) == sizeof(DWORD));

long const MAX_UNICODE_PATH = 32767;

class handle : boost::noncopyable
{
public:
	handle(HANDLE h) : h_(h) {}
	~handle() { if(h_) ::CloseHandle(h_); }
	operator HANDLE() const { return h_; }
private:
	HANDLE h_;
};

void check(DWORD result)
{
	using namespace boost::system;
	if ( !result )
	{
		throw system_error(::GetLastError(), system_category());
	}
}

// NtQueryInformationProcess in NTDLL.DLL
typedef NTSTATUS (NTAPI *pfnNtQueryInformationProcess)(IN HANDLE ProcessHandle, IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation, IN ULONG ProcessInformationLength, OUT PULONG ReturnLength OPTIONAL);

// GetSystemTimes in KERNEL32.DLL
typedef BOOL ( __stdcall * pfnGetSystemTimes)( LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );


// Used in PEB struct
typedef ULONG smPPS_POST_PROCESS_INIT_ROUTINE;

// Used in PEB struct
typedef struct _smPEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} smPEB_LDR_DATA, *smPPEB_LDR_DATA;

// Used in PEB struct
typedef struct _smRTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} smRTL_USER_PROCESS_PARAMETERS, *smPRTL_USER_PROCESS_PARAMETERS;

// Used in PROCESS_BASIC_INFORMATION struct
typedef struct _smPEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	smPPEB_LDR_DATA Ldr;
	smPRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	BYTE Reserved4[104];
	PVOID Reserved5[52];
	smPPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved6[128];
	PVOID Reserved7[1];
	ULONG SessionId;
} smPEB, *smPPEB;

static pfnNtQueryInformationProcess NtQueryInformationProcess = 0;

void enable_debug_privilege(HANDLE process)
{
	HANDLE hToken = 0;
	TOKEN_PRIVILEGES tkp = {};
	check(::OpenProcessToken(process, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken));
	handle token = hToken;

	// Get the LUID for the privilege.
	check(::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid));

	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Set the privilege for this process.
	check(::AdjustTokenPrivileges(token, false, &tkp, 0, NULL, NULL));
}

static const uint32_t priority_map[] =
{
	IDLE_PRIORITY_CLASS,
	BELOW_NORMAL_PRIORITY_CLASS,
	NORMAL_PRIORITY_CLASS,
	ABOVE_NORMAL_PRIORITY_CLASS,
	HIGH_PRIORITY_CLASS,
	REALTIME_PRIORITY_CLASS
};

pid_t start(std::string const& command_line, int priority)
{
	if ( priority < 0 || priority >= PRIORITY_CLASS_COUNT )
	{
		throw std::invalid_argument("invalid priority class");
	}

	std::wstring cmd_line = utils::string_to_wstring(command_line);

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = { 0 };

	DWORD dwCreationFlags = priority_map[priority];
#if ENABLE(ISOLATE_CONSOLE_WINDOWS)
	dwCreationFlags |= CREATE_NEW_CONSOLE;
#endif

	if ( !::CreateProcessW(NULL, (wchar_t*)cmd_line.c_str(), NULL, NULL, false,
			 dwCreationFlags, NULL, NULL, &si, &pi) )
	{
		return 0;
	}
	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);
	return pi.dwProcessId;
}

void kill(pid_t pid)
{
	handle proc = ::OpenProcess(PROCESS_TERMINATE, false, pid);
	check(proc != 0);
//	enable_debug_privilege(proc);
	check( ::TerminateProcess(proc, 0) );
	check( ::WaitForSingleObject(proc, 30 * 1000) );
}

pid_t current()
{
	return ::GetCurrentProcessId();
}

bool is_running(pid_t pid)
{
	handle proc = ::OpenProcess(PROCESS_QUERY_INFORMATION, false, pid);
	if ( proc )
	{
		DWORD exit_code;
		check( ::GetExitCodeProcess(proc, &exit_code) );
		return exit_code == STILL_ACTIVE;
	}
	else
	{
		check(::GetLastError() == ERROR_INVALID_PARAMETER);
		return false;
	}
}

pid_list list()
{
	pid_list result;

	size_t count = 0;
	while ( result.size() == count )
	{
		result.resize(result.size() + 1000);
		DWORD const buf_size = (DWORD)(result.size() * sizeof(pid_t));
		DWORD bytes_returned = 0;
		check( ::EnumProcesses((DWORD*)&result[0], buf_size, &bytes_returned) );
		count = bytes_returned / sizeof(pid_t);
	};
	result.resize(count);
	return result;
}

static inline uint64_t ft_to_uint64(FILETIME const& ft)
{
	return (uint64_t(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

// 100-nanoseconds = milliseconds * 10000
static double const FT_TICKS = 10000;

// FILETIME to Unix timestamp in milliseconds
inline double ft_to_unix_timestamp_ms(FILETIME const& ft)
{
	// Windows Epoch adjust between 01.01.1601 and 01.01.1970
	uint64_t const epoch_adjust = uint64_t(11644473600 * 1000 * FT_TICKS);
	uint64_t const ft64 = ft_to_uint64(ft);
	return ft64? (ft64 - epoch_adjust) / FT_TICKS : 0;
}

static bool process_cpu_usage(v8::Isolate* isolate, HANDLE proc, v8::Handle<v8::Object>& result)
{
	FILETIME kernel = {}, user = {}, creation = {}, exit = {};
	if (GetProcessTimes(proc, &creation, &exit, &kernel, &user))
	{
		set_option(isolate, result, "start",  ft_to_unix_timestamp_ms(creation));
		set_option(isolate, result, "exit",   ft_to_unix_timestamp_ms(exit));
		set_option(isolate, result, "kernel", ft_to_uint64(kernel) / FT_TICKS);
		set_option(isolate, result, "user",   ft_to_uint64(user) /  FT_TICKS);
		set_option(isolate, result, "total",  ft_to_uint64(user) / FT_TICKS + ft_to_uint64(kernel) / FT_TICKS);

		return true;
	}
	return false;
}

void get_info(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	static pfnNtQueryInformationProcess NtQueryInformationProcess = 0;
	if ( !NtQueryInformationProcess )
	{
		HMODULE ntdll = ::LoadLibraryW(L"ntdll.dll"); // don't unload ntdll.dll until program exit
		check(ntdll != 0);
		NtQueryInformationProcess = (pfnNtQueryInformationProcess)::GetProcAddress(ntdll, "NtQueryInformationProcess");
	}

	v8::Isolate* isolate = args.GetIsolate();

	pid_t const pid = v8pp::from_v8<pid_t>(isolate, args[0]);

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Object> result = v8::Object::New(isolate);

	handle proc = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
	bool is_running = (proc != 0);
	if ( proc )
	{
		//enable_debug_privilege(proc);
		/*
		BOOL is_wow64 = false;
		win32_check(::IsWow64Process(proc, &is_wow64));
		PROCESSINFOCLASS const proc_info_class = (is_wow64? ProcessWow64Information : ProcessBasicInformation);
		*/

		PROCESSINFOCLASS const proc_info_class = ProcessBasicInformation;
		PROCESS_BASIC_INFORMATION proc_info = {};

		check( NtQueryInformationProcess(proc, proc_info_class, &proc_info, sizeof(proc_info), NULL) >= 0);

		_aspect_assert(proc_info.UniqueProcessId == pid);
		is_running = ((LONG)proc_info.Reserved1 == STILL_ACTIVE);

		if (proc_info.PebBaseAddress)
		{
			smPEB peb = {};
			SIZE_T bytes_read = 0;
//			check( ::ReadProcessMemory(proc, proc_info.PebBaseAddress, &peb, sizeof(peb), &bytes_read) );

			// changed logic to repeat ReadProcessMemory() as it sometimes fails [2/5/2012 asy]
			if(!::ReadProcessMemory(proc, proc_info.PebBaseAddress, &peb, sizeof(peb), &bytes_read))
			{
				Sleep(1);	// hope this helps...
				check( ::ReadProcessMemory(proc, proc_info.PebBaseAddress, &peb, sizeof(peb), &bytes_read) );
			}
			_aspect_assert(bytes_read == sizeof(peb));

			if ( peb.ProcessParameters )
			{
				smRTL_USER_PROCESS_PARAMETERS proc_params;
//				check( ::ReadProcessMemory(proc, peb.ProcessParameters, &proc_params, sizeof(proc_params), &bytes_read) );
				if(!::ReadProcessMemory(proc, peb.ProcessParameters, &proc_params, sizeof(proc_params), &bytes_read))
				{
					Sleep(1); // hope this helps...
					check( ::ReadProcessMemory(proc, peb.ProcessParameters, &proc_params, sizeof(proc_params), &bytes_read) );
				}
				_aspect_assert(bytes_read == sizeof(proc_params));

				std::vector<wchar_t> str_buf;
				
				str_buf.resize(proc_params.ImagePathName.MaximumLength / sizeof(wchar_t));
				check(::ReadProcessMemory(proc, proc_params.ImagePathName.Buffer,
					&str_buf[0], proc_params.ImagePathName.MaximumLength, NULL));
				set_option<wchar_t const*>(isolate, result, "name", &str_buf[0]);

				str_buf.resize(proc_params.CommandLine.MaximumLength / sizeof(wchar_t));
				check(::ReadProcessMemory(proc, proc_params.CommandLine.Buffer,
					&str_buf[0], proc_params.CommandLine.MaximumLength, NULL));
				set_option<wchar_t const*>(isolate, result, "cmdLine", &str_buf[0]);
			}
		}

		PROCESS_MEMORY_COUNTERS process_memory_counters;
		memset(&process_memory_counters,0,sizeof(process_memory_counters));
		process_memory_counters.cb = sizeof(process_memory_counters);

		if(::GetProcessMemoryInfo(proc, &process_memory_counters, sizeof(process_memory_counters)))
		{
			v8::Local<v8::Object> o = v8::Object::New(isolate);
			set_option(isolate, result, "memory", o);
			set_option(isolate, o, "workingSetSize", static_cast<uint64_t>(process_memory_counters.WorkingSetSize));
			set_option(isolate, o, "pagefileUsage", static_cast<uint64_t>(process_memory_counters.PagefileUsage));
		}
	}
	set_option(isolate, result, "isRunning", is_running);

	v8::Local<v8::Object> cpu_info = v8::Object::New(isolate);
	if (process_cpu_usage(isolate, proc, cpu_info))
	{
		set_option(isolate, result, "cpu", cpu_info);
	}

	args.GetReturnValue().Set(scope.Escape(result));
}

void set_priority(pid_t pid, int priority)
{
	if (priority < 0 || priority >= PRIORITY_CLASS_COUNT)
	{
		throw std::invalid_argument("invalid priority class");
	}

	handle proc = (pid? ::OpenProcess(PROCESS_SET_INFORMATION, false, pid) : ::GetCurrentProcess());
	check(proc != 0);
	check(::SetPriorityClass(proc, priority_map[priority]));
}

void get_cpu_usage(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Object> obj = v8::Object::New(isolate);

	pid_t const pid = v8pp::from_v8<pid_t>(isolate, args[0], 0);
	// if no PID has been supplied, obtain system timing
	if (!pid)
	{
		FILETIME idle, kernel, user;
		check(GetSystemTimes(&idle, &kernel,&user));

		set_option(isolate, obj, "idle",   ft_to_uint64(idle) / FT_TICKS);
		set_option(isolate, obj, "kernel", (ft_to_uint64(kernel) - ft_to_uint64(idle))/ FT_TICKS);
		set_option(isolate, obj, "user",   ft_to_uint64(user) / FT_TICKS);
		set_option(isolate, obj, "total",  (ft_to_uint64(kernel) + ft_to_uint64(user)) / FT_TICKS);
	}
	else
	{
		handle proc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION , false, pid);
		check(proc != 0);
		check(process_cpu_usage(isolate, proc, obj));
	}

	args.GetReturnValue().Set(scope.Escape(obj));
}


#else

void check(int result)
{
	using namespace boost::system;
	if ( result == -1 )
	{
		throw system_error(errno, system_category());
	}
}

int const priority_map[] =
{
	-20,  // PRIORITY_IDLE
	 -5,  // PRIORITY_BELOW_NORMAL
	  0,  // PRIORITY_NORMAL
	  5,  // PRIORITY_ABOVE_NORMAL
	 10,  // PRIORITY_HIGH
	 19,  // PRIORITY_REALTIME
};

pid_t start(std::string const& command_line, int priority)
{
	if (priority < 0 || priority >= PRIORITY_CLASS_COUNT)
	{
		throw std::invalid_argument("invalid priority class");
	}

	pid_t const pid = fork();
	check(pid);
	if ( pid == 0 )
	{
		// child
		set_priority(0, priority_map[priority]);
		execlp(command_line.c_str(), command_line.c_str(), (char*)NULL);
		exit(0);
	}
	return pid;
}

void kill(pid_t pid)
{
	check(::kill(pid, SIGTERM));
	check(::waitpid(pid, NULL, 0));
}

pid_t current()
{
	return getpid();
}

bool is_running(pid_t pid)
{
	return ::kill(pid, 0) == 0;
}

pid_list list()
{
	pid_list result;
#if OS(LINUX)
	using namespace boost::filesystem;
	for (directory_iterator it("/proc"), end; it != end; ++it)
	{
		if ( it->status().type() == directory_file )
		{
			pid_t const pid = atoi(it->path().filename().c_str());
			if ( pid )
			{
				result.push_back(pid);
			}
		}
	}
#elif OS(DARWIN)
	int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
	size_t size = 0;
	if (sysctl(mib, 3, NULL, &size, NULL, 0) == 0 && size > 0)
	{
		std::vector<char> buf(size * 2);
		size = buf.size();
		if (sysctl(mib, 3, &buf[0], &size, NULL, 0) == 0 && size > 0)
		{
			kinfo_proc const* const proc = reinterpret_cast<kinfo_proc*>(&buf[0]);
			result.resize(size / sizeof(kinfo_proc));
			for (size_t i = 0; i < result.size(); ++i)
			{
				result[i] = proc[i].kp_proc.p_pid;
			}
		}
	}
#else
#error Unsupported OS
#endif
	return result;
}

void get_info(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	pid_t pid = v8pp::from_v8<pid_t>(isolate, args[0]);

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Object> result = v8::Object::New(isolate);

#if OS(LINUX)
	using namespace boost::filesystem;

	path const proc_dir = "/proc" /  boost::lexical_cast<std::string>(pid);

	ifstream stat(proc_dir / "stat");
	bool is_running = false;
	std::string name, cmd_line;
	if ( stat )
	{
		char state = 0;
		stat >> pid >> name >> state;
		is_running = (state != 0);
	}

	ifstream cmdline(proc_dir / "cmdline");
	std::getline(cmdline, cmd_line, '\0');

	set_option(isolate, result, "isRunning", is_running);
	set_option(isolate, result, "name", name);
	set_option(isolate, result, "cmdLine", cmd_line);
#elif OS(DARWIN)
	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
	kinfo_proc info = {};
	size_t size = sizeof(info);
	if (sysctl(mib, 4, &info, &size, NULL, 0) == 0)
	{
		set_option(isolate, result, "isRunning", info.kp_proc.p_stat != 0);
		set_option(isolate, result, "name", info.kp_proc.p_comm);
	}
	mib[1] = KERN_ARGMAX;
	size_t argmax = 0;
	size = sizeof(argmax);
	if (sysctl(mib, 2, &argmax, &size, NULL, 0) == 0 && argmax > 0)
	{
		std::string args_buf(argmax, 0);
		mib[1] = KERN_PROCARGS2;
		mib[2] = pid;
		if (sysctl(mib, 3, &args_buf[0], &argmax, NULL, 0) == 0)
		{
			args_buf.resize(argmax);
			char const* arg_ptr = &args_buf[0];
			char const* const arg_end = arg_ptr + argmax;
			// arg count
			int num_args;
			memcpy(&num_args, arg_ptr, sizeof(num_args));
			arg_ptr += sizeof(num_args);

			// split args_buf to strings delimited by '\0'
			std::vector<std::string> args;

			boost::iterator_range<char const*> input(arg_ptr, arg_end);

			using namespace boost::algorithm;
			split(args, input, is_from_range('\0', '\0'), token_compress_on);

			// process strings
			std::vector<std::string>::const_iterator begin = args.begin(), end = args.end();
			if (begin != end && begin->empty()) ++begin;
			if (begin != end && args.back().empty()) --end;

			if (std::distance(begin, end) > num_args)
			{
				// first string is a process name
				set_option(isolate, result, "name", *begin);
				++begin;
			}
			// next num_args strings are command line arguments
			num_args = std::min<int>(num_args, std::distance(begin, end));
			if (num_args > 0)
			{
				std::vector<std::string> cmd_line(begin, begin + num_args);
				set_option(isolate, result, "cmdLine", cmd_line);
				begin += num_args;
			}
			// rest of strings are environment variables
			if (begin != end)
			{
				v8::Local<v8::Object> env = v8::Object::New(isolate);
				for (; begin != end; ++begin)
				{
					std::string const& str = *begin;
					std::string::size_type const name_len = str.find('=');
					v8::Local<v8::String> name = v8pp::to_v8(isolate, str.data(), name_len);
					v8::Local<v8::String> value;
					if (name_len == str.npos)
					{
						value = v8pp::to_v8(isolate, "", 0);
					}
					else
					{
						value = v8pp::to_v8(isolate, str.data() + name_len + 1, str.length() - name_len - 1);
					}
					env->Set(name, value);
				}
				set_option(isolate, result, "env", env);
			}
		}
	}

	set_option(isolate, result, "memory", v8::Object::New(isolate));
	set_option(isolate, result, "cpu", v8::Object::New(isolate));
#else
#error Unsupported OS
#endif
	args.GetReturnValue().Set(scope.Escape(result));
}

void set_priority(pid_t pid, int priority)
{
	if (priority < 0 || priority >= PRIORITY_CLASS_COUNT)
	{
		throw std::invalid_argument("invalid priority class");
	}

	check(setpriority(PRIO_PROCESS, pid, priority_map[priority]));
}

void get_cpu_usage(v8::FunctionCallbackInfo<v8::Value> const&)
{
	throw std::runtime_error("not implemented");
}

#endif

bool signal_named_condition(std::string const& condition_name)
{
#if OS(WINDOWS)
	HANDLE h = ::CreateEventA(NULL, TRUE, FALSE, condition_name.c_str());
	if( h == INVALID_HANDLE_VALUE )
	{
		return false;
	}
	bool const success = (SetEvent(h) != 0);
	CloseHandle(h);
	return success;
#else
	_aspect_assert(false);
	throw std::runtime_error("signal_named_condition() is not implemented");
#endif
}

}} // aspect::process
