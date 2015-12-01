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

#if OS(WINDOWS)

#include <strsafe.h>
#include <stdio.h>
#include <dbghelp.h>

namespace aspect { namespace os {

//void set_thread_exception_handlers();
//void set_process_exception_handler(wchar_t const* app_name, wchar_t const* report_contact);

static size_t const MAX_APP_NAME_LEN = 41;
static wchar_t app_name_[MAX_APP_NAME_LEN] = {};

static size_t const MAX_REPORT_CONTACT_LEN = 141;
static wchar_t report_contact_[MAX_REPORT_CONTACT_LEN] = {};

static EXCEPTION_POINTERS* exception_pointers_ = nullptr;

static bool set_dump_privileges()
{
	// This method is used to have the current process be able to call MiniDumpWriteDump
	// This code was taken from:
	// http://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/f54658a4-65d2-4196-8543-7e71f3ece4b6/


	bool result = false;

	HANDLE token_handle = NULL;
	if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_handle))
	{
		goto cleanup;
	}

	TOKEN_PRIVILEGES token_privileges;
	token_privileges.PrivilegeCount = 1;

	if (!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &token_privileges.Privileges[0].Luid))
	{
		goto cleanup;
	}

	token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!::AdjustTokenPrivileges(token_handle, false,
		&token_privileges, sizeof(token_privileges), NULL, NULL))
	{
		goto cleanup;
	}

	result = true;

cleanup:

	if (token_handle)
	{
		::CloseHandle(token_handle);
	}

	return result;
}

static void create_minidump()
{
	HANDLE dumpfile = INVALID_HANDLE_VALUE;

	if (!set_dump_privileges())
	{
		fputws(L"Unable to adjust privileges to create minidump\n", stderr);
		goto cleanup;
	}

	DWORD const buf_size = MAX_PATH * 2;
	wchar_t dump_filename[buf_size];

	::GetTempPathW(buf_size, dump_filename);
	StringCbCat(dump_filename, buf_size, *app_name_? app_name_ : L"app");
	StringCbCat(dump_filename, buf_size, L"-crash.dmp");

	dumpfile = ::CreateFile(dump_filename, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dumpfile == INVALID_HANDLE_VALUE)
	{
		fputws(L"Unable to create minidump file ", stderr);
		fputws(dump_filename, stderr);
		goto cleanup;
	}

	MINIDUMP_TYPE const dump_type = MiniDumpWithFullMemory;//MiniDumpNormal;

	MINIDUMP_EXCEPTION_INFORMATION dump_exception_info;
	dump_exception_info.ThreadId = ::GetCurrentThreadId();
	dump_exception_info.ExceptionPointers = exception_pointers_;
	dump_exception_info.ClientPointers = false;

	if (!::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(),
		dumpfile, dump_type, &dump_exception_info, nullptr, nullptr))
	{
		fputws(L"Unable to write minidump file ", stderr);
		fputws(dump_filename, stderr);
		goto cleanup;
	}

	fputws(L"Minidump has been written to ", stderr);
	fputws(dump_filename, stderr);
	if (*report_contact_)
	{
		fputws(L"Please send it to ", stderr);
		fputws(report_contact_, stderr);
	}

cleanup:
	if (dumpfile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(dumpfile);
	}
}

static void get_exception_pointers(EXCEPTION_POINTERS* ep)
{
	// The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)

	CONTEXT context = {};
	ZeroMemory(&context, sizeof(context));

#ifdef _X86_

	__asm {
		mov dword ptr [context.Eax], eax
		mov dword ptr [context.Ecx], ecx
		mov dword ptr [context.Edx], edx
		mov dword ptr [context.Ebx], ebx
		mov dword ptr [context.Esi], esi
		mov dword ptr [context.Edi], edi
		mov word ptr [context.SegSs], ss
		mov word ptr [context.SegCs], cs
		mov word ptr [context.SegDs], ds
		mov word ptr [context.SegEs], es
		mov word ptr [context.SegFs], fs
		mov word ptr [context.SegGs], gs
		pushfd
		pop [context.EFlags]
	}

	context.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable:4311)
	context.Eip = (ULONG)_ReturnAddress();
	context.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
	context.Ebp = *((ULONG *)_AddressOfReturnAddress()-1);

#elif defined (_IA64_) || defined (_AMD64_)

	/* Need to fill up the Context in IA64 and AMD64. */
	RtlCaptureContext(&context);

#else  /* defined (_IA64_) || defined (_AMD64_) */

#endif  /* defined (_IA64_) || defined (_AMD64_) */

	CopyMemory(ep->ContextRecord, &context, sizeof(CONTEXT));
	ZeroMemory(ep->ExceptionRecord, sizeof(EXCEPTION_RECORD));

	//ep->ExceptionRecord->ExceptionCode = dwExceptionCode;
	ep->ExceptionRecord->ExceptionAddress = _ReturnAddress();
}

static void generate_report()
{
	EXCEPTION_RECORD local_exception_record;
	CONTEXT local_context;

	EXCEPTION_POINTERS local_exception_pointers;

	local_exception_pointers.ExceptionRecord = &local_exception_record;
	local_exception_pointers.ContextRecord = &local_context;

	if (!exception_pointers_)
	{
		get_exception_pointers(&local_exception_pointers);
		exception_pointers_ = &local_exception_pointers;
	}

	create_minidump();
	::TerminateProcess(::GetCurrentProcess(), 100500);
}

static DWORD WINAPI generate_report_thread(void*)
{
	generate_report();
	return 0;
}

static LONG WINAPI handle_seh(EXCEPTION_POINTERS* ep)
{
#ifdef _DEBUG
	return EXCEPTION_CONTINUE_SEARCH;
#else
	fputs("FATAL: unhandled exception.\n", stderr);

	exception_pointers_ = ep;

	if (ep && ep->ExceptionRecord && ep->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		// generate stack overflow report in a new thread
		HANDLE thread = ::CreateThread(0, 0, generate_report_thread, nullptr, 0, nullptr);
		::WaitForSingleObject(thread, 600 * 1000);
		::CloseHandle(thread);
	}
	else
	{
		generate_report();
	}

	// never reach this
	return EXCEPTION_EXECUTE_HANDLER;
#endif
}

static void handle_terminate()
{
	fputs("FATAL: terminate() called.\n", stderr);
	exception_pointers_ = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

	generate_report();
}

static void handle_unexpected()
{
	fputs("FATAL: unexpected() called.\n", stderr);
	exception_pointers_ = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

	generate_report();
}

static void handle_purecall()
{
	fputs("FATAL: Pure function called.\n", stderr);
	exception_pointers_ = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

	generate_report();
}

#if _MSC_VER>=1400
static void handle_invalid_parameter(wchar_t const* /*expression*/, wchar_t const* /*function*/,
	wchar_t const* /*file*/, unsigned int /*line*/, uintptr_t /*reserved*/)
{
	fputs("FATAL: invalid parameter for CRT function.\n", stderr);
	exception_pointers_ = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

	generate_report();
}
#endif

static void handle_signal(int signal)
{
	char const* signal_name = nullptr;
	switch (signal)
	{
	case SIGINT:   signal_name = "SIGINT"; break;
	case SIGILL:   signal_name = "SIGILL"; break;
	case SIGFPE:   signal_name = "SIGFPE"; break;
	case SIGSEGV:  signal_name = "SIGSEGV"; break;
	case SIGTERM:  signal_name = "SIGTERM"; break;
	case SIGBREAK: signal_name = "SIGBREAK"; break;
	case SIGABRT:  signal_name = "SIGABRT"; break;
	}

	fputs("FATAL: signal ", stderr);
	if (signal_name)
	{
		fprintf(stderr, "%s.\n", signal_name);
	}
	else
	{
		fprintf(stderr, "%d.\n", signal);
	}

	exception_pointers_ = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

	generate_report();
}

void set_thread_exception_handlers()
{
	set_terminate(handle_terminate);
	set_unexpected(handle_unexpected);
	signal(SIGABRT, handle_signal);
	signal(SIGFPE, handle_signal);
	signal(SIGILL, handle_signal);
	signal(SIGSEGV, handle_signal);
}

void CORE_API set_process_exception_handler(wchar_t const* app_name, wchar_t const* report_contact)
{
	if (app_name)
	{
		wcsncpy(app_name_, app_name, MAX_APP_NAME_LEN);
	}
	else
	{
		wchar_t filename[MAX_PATH + 1];

		GetModuleFileNameW(NULL, filename, MAX_PATH);
		_wsplitpath(filename, nullptr, nullptr, filename, nullptr);
		wcsncpy(app_name_, filename, MAX_APP_NAME_LEN);
	}

	if (report_contact)
	{
		wcsncpy(report_contact_, report_contact, MAX_REPORT_CONTACT_LEN);
	}

	// SEH handler
	::SetUnhandledExceptionFilter(handle_seh);

#if _MSC_VER>=1400
	// Visual C++ CRT handlers
	_set_error_mode(_OUT_TO_STDERR);

	_set_purecall_handler(handle_purecall);
	//_set_new_handler(1);

	_set_invalid_parameter_handler(handle_invalid_parameter);

	_set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
#endif

	set_thread_exception_handlers();
}

}} // aspect::os

#endif // OS(WINDOWS)
