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
#include "jsx/os.hpp"

#include "jsx/cpus.hpp"
#include "jsx/runtime.hpp"

#if OS(WINDOWS)
#include <iptypes.h>
#include <iphlpapi.h>
#include <shellapi.h>
#endif

#if OS(UNIX)
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <ifaddrs.h>
#endif

#if OS(DARWIN)
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach-o/dyld.h>
#endif

#include <fstream>
#include <boost/algorithm/string.hpp>

namespace aspect { namespace os {

using namespace v8;

#if OS(WINDOWS)
std::string win32_error_message(int code)
{
	boost::system::error_code const ec(code, boost::system::get_system_category());
	return ec.message();
}

std::string read_reg_value(HKEY root, char const* key_name, char const* value_name)
{
	std::string result;
	DWORD size = 0;
	::RegGetValueA(root, key_name, value_name, RRF_RT_REG_SZ, nullptr, nullptr, &size);
	if (size > 0)
	{
		result.resize(size);
		::RegGetValueA(root, key_name, value_name, RRF_RT_REG_SZ, nullptr, &result[0], &size);
		result.resize(size - 1);
	}
	return result;
}

#elif OS(LINUX)
std::string read_proc_fs_value(std::string line)
{
	line = line.substr(line.find(':') + 1);
	boost::algorithm::trim(line);
	return line;
}

uint64_t parse_meminfo_value(std::string line)
{
	using namespace boost::algorithm;

	line = read_proc_fs_value(line);
	to_upper(line);

	uint64_t units = 1;
	if (ends_with(line, "KB"))
	{
		line.resize(line.size() - 2);
		units = 1024;
	}
	else if (ends_with(line, "MB"))
	{
		line.resize(line.size() - 2);
		units = 1024 * 1024;
	}
	else if (ends_with(line, "GB"))
	{
		line.resize(line.size() - 2);
		units = 1024 * 1024 * 1024;
	}
	trim(line);
	return boost::lexical_cast<uint64_t>(line) * units;
}
#endif

boost::filesystem::path exe_path()
{
// http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe

	boost::filesystem::path result;
#if OS(WINDOWS)
	std::string buf(2048, 0);
	size_t size = ::GetModuleFileNameA(NULL, &buf[0], static_cast<DWORD>(buf.size()));
	if (size > buf.size())
	{
		buf.resize(size + 1);
		size = ::GetModuleFileNameA(NULL, &buf[0], static_cast<DWORD>(buf.size()));
	}
	buf.resize(size);
	//std::transform(buf.begin(), buf.end(), buf.begin(), tolower);
	result = buf.c_str();
#elif OS(LINUX)
	result = boost::filesystem::read_symlink("/proc/self/exe");
#elif OS(FREEBSD)
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	std::string buf(2048, 0);
	size_t size = buf.size();
	if (sysctl(mib, 4, &buf[0], &size, NULL, 0) == -1)
	{
		buf.resize(size + 1);
		sysctl(mib, 4, &buf[0], &size, NULL, 0)
	}
	buf.resize(size);
	result = buf.c_str();
#elif OS(DARWIN)
	std::string buf(2048, 0);
	uint32_t size = static_cast<uint32_t>(buf.size());
	if ( _NSGetExecutablePath(&buf[0], &size) == -1 )
	{
		buf.resize(size + 1);
		_NSGetExecutablePath(&buf[0], &size);
	}
	buf.resize(size);
	result = buf.c_str();
#else
#error Not implemented yet
#endif
	return resolve(result);
}

boost::filesystem::path resolve(const boost::filesystem::path& p)
{
	boost::filesystem::path result;
	for(boost::filesystem::path::iterator it=p.begin();
		it!=p.end();
		++it)
	{
		if(*it == "..")
		{
			// /a/b/.. is not necessarily /a if b is a symbolic link
			if(boost::filesystem::is_symlink(result) )
				result /= *it;
			// /a/b/../.. is not /a/b/.. under most circumstances
			// We can end up with ..s in our result because of symbolic links
			else if(result.filename() == "..")
				result /= *it;
			// Otherwise it should be safe to resolve the parent
			else
				result = result.parent_path();
		}
		else if(*it == ".")
		{
			// Ignore
		}
		else
		{
			// Just cat other path entries
			result /= *it;
		}
	}
	result.make_preferred();
	return result;
}

std::string computer_name()
{
	std::string result;

#if OS(WINDOWS)
	result.resize(MAX_COMPUTERNAME_LENGTH + 1);
	DWORD size = static_cast<DWORD>(result.size());
	GetComputerNameExA(ComputerNameNetBIOS, &result[0], &size);
	if (size > result.size())
	{
		result.resize(size + 1);
		GetComputerNameExA(ComputerNameNetBIOS, &result[0], &size);
	}
	result.resize(size);
#else
	result.resize(2048);
	gethostname(&result[0], static_cast<int>(result.size()));
	result = result.c_str();
#endif

	return result;
}

std::string fqdn()
{
	std::string result;

#if OS(WINDOWS)
	result.resize(2048);
	DWORD size = static_cast<DWORD>(result.size());
	GetComputerNameExA(ComputerNameDnsFullyQualified, &result[0], &size);
	if (size > result.size())
	{
		result.resize(size + 1);
		GetComputerNameExA(ComputerNameDnsFullyQualified, &result[0], &size);
	}
	result.resize(size);
#else
	result = computer_name();
	std::string domain_name(2048, 0);
	getdomainname(&domain_name[0], static_cast<int>(domain_name.size()));
	domain_name = domain_name.c_str();
	if (!domain_name.empty())
	{
		result += '.';
		result += domain_name;
	}
#endif

	return result;
}

std::string version()
{
	std::string result;

#if OS(WINDOWS)
	char const* const key_name = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
	result += read_reg_value(HKEY_LOCAL_MACHINE, key_name, "ProductName");
	result += " version ";
	result += read_reg_value(HKEY_LOCAL_MACHINE, key_name, "CurrentVersion");
	result += '.';
	result += read_reg_value(HKEY_LOCAL_MACHINE, key_name, "CurrentBuild");
	result += ' ';
	result += read_reg_value(HKEY_LOCAL_MACHINE, key_name, "CSDVersion");
#elif OS(UNIX)
	utsname name;
	if (uname(&name) == 0)
	{
		result += name.sysname;
		result += ' ';
		result += name.release;
		result += ' ';
		result += name.version;
	}
#else
#error "Unsupported OS"
#endif

	return result;
}

#if OS(WINDOWS)
//
// Usage: SetThreadName (-1, "MainThread");
//
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
#pragma pack(pop)

static void _set_thread_name( uint32_t thread_id, const char *thread_name)
{
#if TARGET(DEBUG)
	// thread name should be max 32 characters
	// and should contain thread name abbreviation
	_aspect_assert(strlen(thread_name) < 32);
#endif

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = thread_name;
	info.dwThreadID = (DWORD)thread_id;
	info.dwFlags = 0;

	__try
	{
		RaiseException( 0x406D1388, 0, /*sizeof(info)/sizeof(DWORD)*/ 4, (const ULONG_PTR*)&info );
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

void set_thread_name(const char *thread_name)
{
	_set_thread_name(GetCurrentThreadId(), thread_name);
}

#elif OS(LINUX)
void set_thread_name(const char *thread_name)
{
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 12)
	pthread_setname_np(pthread_self(), thread_name);
#endif
}
#elif OS(DARWIN)
void set_thread_name(const char *thread_name)
{
	pthread_setname_np(thread_name);
}
#else
void set_thread_name(const char *thread_name) { }
#endif

static void get_info(v8::PropertyCallbackInfo<v8::Value> const& info)
{
	v8::Isolate* isolate = info.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> o = Object::New(isolate);
	v8::Local<v8::Object> os = Object::New(isolate);
	v8::Local<v8::Object> cpu = Object::New(isolate);

	set_option(isolate, o, "os", os);
	set_option(isolate, o, "cpu", cpu);
	set_option(isolate, o, "name", computer_name());
	set_option(isolate, o, "fqdn", fqdn());

	set_option(isolate, os, "type", _OS_NAME);
	set_option(isolate, os, "version", version());

	set_option(isolate, cpu, "cores", cpu_count());
	set_option(isolate, cpu, "type", cpu_type());

	info.GetReturnValue().Set(scope.Escape(o));
}

static int exec(std::string const& cmd)
{
	return system(cmd.c_str());
}

static void get_storage_space(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	std::string const path = v8pp::from_v8<std::string>(isolate, args[0]);
	boost::filesystem::space_info const info = boost::filesystem::space(path);

	v8::Local<v8::Object> o = Object::New(isolate);
	set_option(isolate, o, "total",     info.capacity);
	set_option(isolate, o, "free",      info.free);
	set_option(isolate, o, "available", info.available);

	args.GetReturnValue().Set(scope.Escape(o));
}

static void get_memory_info(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> o = Object::New(isolate);

#if OS(WINDOWS)
	MEMORYSTATUSEX ms = {};
	ms.dwLength = sizeof(ms);
	GlobalMemoryStatusEx(&ms);

	set_option(isolate, o, "totalPhysical", ms.ullTotalPhys);
	set_option(isolate, o, "freePhysical",  ms.ullAvailPhys);
	set_option(isolate, o, "totalSwap",     ms.ullTotalPageFile);
	set_option(isolate, o, "freeSwap",      ms.ullAvailPageFile);
	set_option(isolate, o, "totalVirtual",  ms.ullTotalVirtual);
	set_option(isolate, o, "freeVirtual",   ms.ullAvailVirtual);
#elif OS(LINUX)
	std::ifstream meminfo("/proc/meminfo");

	size_t total_virtual = 0;

	for (std::string s; meminfo;)
	{
		std::getline(meminfo, s);

		using namespace boost::algorithm;
		if (starts_with(s, "MemTotal"))
		{
			set_option(isolate, o, "totalPhysical", parse_meminfo_value(s));
		}
		else if (starts_with(s, "MemFree"))
		{
			set_option(isolate, o, "freePhysical", parse_meminfo_value(s));
		}
		else if (starts_with(s, "SwapTotal"))
		{
			set_option(isolate, o, "totalSwap", parse_meminfo_value(s));
		}
		else if (starts_with(s, "SwapFree"))
		{
			set_option(isolate, o, "freeSwap", parse_meminfo_value(s));
		}
		else if (starts_with(s, "VmallocTotal"))
		{
			total_virtual = parse_meminfo_value(s);
			set_option(isolate, o, "totalVirtual", total_virtual);
		}
		else if (starts_with(s, "VmallocUsed"))
		{
			_aspect_assert(total_virtual);
			set_option(isolate, o, "freeVirtual", total_virtual - parse_meminfo_value(s));
		}
	}
#elif OS(DARWIN)
	uint64_t total_mem;
	size_t size = sizeof(total_mem);
	int which[] = { CTL_HW, HW_MEMSIZE };
	if (sysctl(which, 2, &total_mem, &size, NULL, 0) == 0)
	{
		set_option(isolate, o, "totalPhysical", total_mem);
	}

	vm_statistics_data_t vm_stat;
	mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(integer_t);
	if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vm_stat, &count) == 0)
	{
		uint64_t const free_mem = vm_stat.free_count * sysconf(_SC_PAGESIZE);
		set_option(isolate, o, "freePhysical", free_mem);
	}

	xsw_usage swap_usage;
	size = sizeof(swap_usage);
	if (sysctlbyname("vm.swapusage", &swap_usage, &size, NULL, 0) == 0)
	{
		set_option(isolate, o, "totalSwap", swap_usage.xsu_total);
		set_option(isolate, o, "freeSwap", swap_usage.xsu_avail);
	}

	task_basic_info info;
	mach_msg_type_number_t info_count = TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &info_count) == 0)
	{
		set_option(isolate, o, "totalVirtual", info.virtual_size);
		set_option(isolate, o, "freeVirtual", info.virtual_size - info.resident_size);
	}
#else
#error "Unsupported OS"
#endif

	args.GetReturnValue().Set(scope.Escape(o));
}

static boost::asio::ip::address sockaddr_to_ip_address(sockaddr const& sa)
{
	using namespace boost::asio::ip;

	switch (sa.sa_family)
	{
	case AF_INET:
		{
			sockaddr_in const* sin = reinterpret_cast<sockaddr_in const*>(&sa);
			return address_v4(ntohl(sin->sin_addr.s_addr));
		}
		break;
	case AF_INET6:
		{
			sockaddr_in6 const* sin6 = reinterpret_cast<struct sockaddr_in6 const*>(&sa);
			address_v6::bytes_type bytes;
			std::copy(boost::begin(sin6->sin6_addr.s6_addr), boost::end(sin6->sin6_addr.s6_addr), bytes.begin());
			return address_v6(bytes, sin6->sin6_scope_id);
		}
		break;
	default:
		return boost::asio::ip::address();
	}
}

static v8::Handle<Value> ip_address_info(v8::Isolate* isolate, boost::asio::ip::address const& address)
{
	if (address.is_unspecified())
	{
		return v8::Undefined(isolate);
	}

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> info = Object::New(isolate);

	set_option(isolate, info, "family", address.is_v6()? "IPv6" : "IPv4");
	set_option(isolate, info, "address", address.to_string());
	set_option(isolate, info, "isLoopback", address.is_loopback());
	set_option(isolate, info, "isMulticast", address.is_multicast());

	return scope.Escape(info);
}

void get_network_interfaces(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8::Local<Object> result = Object::New(isolate);

#if OS(WINDOWS)
	ULONG const family = AF_UNSPEC;
	ULONG const flags = GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST;
	ULONG buf_size = 0;
	::GetAdaptersAddresses(family, flags, nullptr, nullptr, &buf_size);

	if (buf_size > 0)
	{
		std::vector<char> buf(buf_size);
		IP_ADAPTER_ADDRESSES* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(&buf[0]);
		ULONG const err = ::GetAdaptersAddresses(family, flags, nullptr, addresses, &buf_size);
		if (err != ERROR_SUCCESS)
		{
			boost::system::error_code const ec(static_cast<int>(err), boost::system::get_system_category());
			throw boost::system::system_error(ec, "get_network_interfaces");
		}

		for (IP_ADAPTER_ADDRESSES* adapter = addresses; adapter; adapter = adapter->Next)
		{
			if (!adapter->FirstUnicastAddress || adapter->OperStatus != IfOperStatusUp)
			{
				// no unicast address or adapter isn't in up state
				continue;
			}

			v8::Local<v8::Array> addrs = Array::New(isolate);
			uint32_t addrs_count = 0;
			for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address; address = address->Next)
			{
				if (!address->Address.lpSockaddr)
				{
					continue;
				}

				boost::asio::ip::address const ip_addr = sockaddr_to_ip_address(*address->Address.lpSockaddr);
				if (ip_addr.is_unspecified())
				{
					continue;
				}

				addrs->Set(addrs_count++, ip_address_info(isolate, ip_addr));
			}
			for (IP_ADAPTER_MULTICAST_ADDRESS* address = adapter->FirstMulticastAddress; address; address = address->Next)
			{
				if (!address->Address.lpSockaddr)
				{
					continue;
				}

				boost::asio::ip::address const ip_addr = sockaddr_to_ip_address(*address->Address.lpSockaddr);
				if (ip_addr.is_unspecified())
				{
					continue;
				}

				addrs->Set(addrs_count++, ip_address_info(isolate, ip_addr));
			}
			result->Set(v8pp::to_v8(isolate, static_cast<wchar_t const*>(adapter->FriendlyName)), addrs);
		}
	}
#elif OS(UNIX)
	ifaddrs* addresses;
	if (getifaddrs(&addresses) == -1)
	{
		boost::system::error_code const ec(errno, boost::system::get_system_category());
		throw boost::system::system_error(ec, "get_network_interfaces");
	}

	for (ifaddrs* address = addresses; address; address = address->ifa_next)
	{
		if (address->ifa_addr == nullptr || (address->ifa_flags & IFF_UP) == 0)
		{
			continue;
		}

		boost::asio::ip::address const ip_addr = sockaddr_to_ip_address(*address->ifa_addr);
		if (ip_addr.is_unspecified())
		{
			continue;
		}

		v8::Local<v8::Array> addrs;
		if (!get_option(isolate, result, address->ifa_name, addrs))
		{
			addrs = Array::New(isolate);
			set_option(isolate, result, address->ifa_name, addrs);
		}
		addrs->Set(addrs->Length(), ip_address_info(isolate, ip_addr));
	}

	freeifaddrs(addresses);
#else
#error "Unsupported OS"
#endif

	args.GetReturnValue().Set(scope.Escape(result));
}

static void chdir(std::string const& new_dir)
{
	boost::filesystem::current_path(new_dir);
}

static bool reboot()
{
#if OS(WINDOWS)
	HANDLE hToken;              // handle to process token
	TOKEN_PRIVILEGES tkp;       // pointer to token structure

	// Get the current process token
	// handle so we can get shutdown privilege.
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		//out_strError.Format("OpenProcessToken() failed: %s",
		//	(char*)GAPI::Win32ErrorMessage(GetLastError()));
		return false;
	}
	// Get the LUID for shutdown privilege.
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

	// Acquire shutdown privilege for this process.
	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(
		hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);
	if (GetLastError() != ERROR_SUCCESS)
	{
		//out_strError.Format("AdjustTokenPrivileges() failed: %s",
		//	(char*)GAPI::Win32ErrorMessage(GetLastError()));
		return false;
	}
	// Reboot!
	if (!::InitiateSystemShutdownEx(
		NULL, NULL, 0, TRUE, TRUE,
		SHTDN_REASON_MAJOR_OTHER
		| SHTDN_REASON_MINOR_OTHER
		| SHTDN_REASON_FLAG_PLANNED))
	{
// 		out_strError.Format("Reboot failed: %s",
// 			(char*)GAPI::Win32ErrorMessage(GetLastError()));
		return false;
	}
#elif OS(UNIX)
	::reboot(RB_AUTOBOOT);
#else
// #error Not implemented yet
	throw std::runtime_error("This function is not implemented under this operating system.");
#endif

	// Yes, we do return from this call (reboot is not immediate)
	return true;
}

void setup_bindings(v8pp::module& target)
{
	/**
	@module os OS
	Contains several operating system related functions.
	**/
	v8pp::module os_template(target.isolate());

	os_template
		/**
		@property info {Object}
		A read-only object with information about the operating system.
		Information object has following properties:

		  * `os`
			* `type`     Operating system type, same as runtime#rt.platform
			* `version`  OS version string
		  * `cpu`
			* `cores`    Number of processor cores
			* `type`     Processor type description string
		  * `name`       Computer name
		  * `fqdn`       Fully qualified computer name
		**/
		.set("info", v8pp::property(get_info))

		/**
		@function reboot()
		@return {Boolean}
		Reboot the system. Requires appropriate privileges, return true on success.
		**/
		.set("reboot", reboot)

		/**
		@function exec(cmd)
		@param cmd {String}
		@return {Number} `cmd` exit code
		Execute `cmd` string
		**/
		.set("exec", exec)

		/**
		@function chdir(new_dir)
		@param new_dir {String}
		Change current directory.
		Has the same effect as runtime#rt.local.currentPath changing.
		**/
		.set("chdir", chdir)

		/**
		@function getStorageSpace(path)
		@param path {String}
		@return {Object}
		Return an object with storage space information in the filesystem at specified path.
		Storage space information object has follwing properties:
		  * `total`      Total number ob bytes.
		  * `free`       Number of free bytes.
		  * `available`  Number of free bytes available to non-privileged process.
		**/
		.set("getStorageSpace", get_storage_space)

		/**
		@function getMemoryInfo()
		@return {Object}
		Return an object with information about memory usage. The memory
		information objecthas following properties:

		  * `totalPhysical` The amount of physical memory in bytes.
		  * `freePhysical`  The amount of currently available physical memory in bytes.
		  * `totalSwap`     Swap file size limit in bytes for the current process.
		  * `freeSwap`      Swap size available to commit for the current process.
		  * `totalVirtual`  The total amount of virtual memory in bytes for the current process.
		  * `freeVirtual`   The amount of available virtual memory in bytes for the current process.
		**/
		.set("getMemoryInfo", get_memory_info)

		/**
		@function getNetworkInterfaces()
		@return {Object}
		Return an object with network interfaces description. Each property in the
		result object is a network adapter with array of addresses. Each address is an
		object with following properties:

		  * `family` IP address family, either `IPv4` or `IPv6`.
		  * `address` IP address string
		  * `isLoopback` Boolean flag is the address a loopback address
		  * `isMulticast` Boolean flag is the address a multicast address

		For example:

		    { 'eth0' : [
		      { 'family' : 'IPv4', 'address' : '192.168.1.2',
		        'isLoopback' : false, 'isMulticast' : false } ]
		      'lo0' : [
		      { 'family' : 'IPv4', 'address' : '127.0.0.1',
		        'isLoopback' : true, 'isMulticast' : false },
		      { 'family' : 'IPv6', 'address' : '::1',
		        'isLoopback' : true, 'isMulticast' : false } ]
		    }
		**/
		.set("getNetworkInterfaces", get_network_interfaces)
		;

	target.set("os", os_template);
}

}} // aspect::os
