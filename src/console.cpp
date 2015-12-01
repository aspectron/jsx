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
#include "jsx/console.hpp"

#include <cassert>
#include <boost/container/flat_map.hpp>

#include "jsx/v8_core.hpp"
#include "jsx/runtime.hpp"

namespace aspect { namespace console {

#if OS(WINDOWS)

class console_state
{
	typedef boost::container::flat_map<HANDLE, console_state> state_map;
public:

	uint8_t foreground, background;
	bool bold:1, underline:1, rvideo:1, concealed:1;

	COORD cursor;

	// create state for console
	explicit console_state(HANDLE console)
	{
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(console, &info);

		foreground = (info.wAttributes & 0x07);
		background = ((info.wAttributes & 0x70) >> 4);
		bold = (info.wAttributes & 0x8) != 0;
		underline = (info.wAttributes & 0x80) != 0;
		rvideo = false;
		concealed = false;

		cursor = info.dwCursorPosition;
	}

	static console_state const& default_state(HANDLE console)
	{
		static state_map defaults;
		state_map::iterator it = defaults.find(console);
		if (it == defaults.end())
		{
			it = defaults.insert(it, std::make_pair(console, console_state(console)));
		}
		return it->second;
	}

	static console_state& current_state(HANDLE console)
	{
		static state_map current;
		state_map::iterator it = current.find(console);
		if (it == current.end())
		{
			console_state const& initial = default_state(console);
			it = current.insert(it, std::make_pair(console, initial));
		}
		return it->second;
	}

	WORD attributes() const
	{
		if (concealed)
		{
			if (rvideo)
			{
				return foreground | (foreground << 4)
					| (bold? FOREGROUND_INTENSITY | BACKGROUND_INTENSITY : 0);
			}
			else
			{
				return background | (background << 4)
					| (underline? FOREGROUND_INTENSITY | BACKGROUND_INTENSITY : 0);
			}
		}
		else if (rvideo)
		{
			return background | (foreground << 4)
				| (bold? BACKGROUND_INTENSITY : 0)
				| (underline? FOREGROUND_INTENSITY : 0);
		}
		else
		{
			return foreground | (background << 4)
				| (bold? FOREGROUND_INTENSITY : 0)
				| (underline? BACKGROUND_INTENSITY : 0);
		}
	}

	static uint8_t ansi_color(unsigned clr)
	{
		static const uint8_t rgb[] = { FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE };

		return (clr & 1? rgb[0] : 0) | (clr & 2? rgb[1] : 0) | (clr & 4? rgb[2] : 0);
	}
};

void println(char const* line, color fg, color bg, attribute attr)
{
	HANDLE const console = GetStdHandle(STD_OUTPUT_HANDLE);
	console_state const& state = console_state::current_state(console);

	uint8_t const foreground = (fg < MAX_COLORS? state.ansi_color(fg) : state.foreground);
	uint8_t const background = (bg < MAX_COLORS? state.ansi_color(bg) : state.background);

	WORD text_attr = foreground | (background << 4);
	switch (attr)
	{
	case BRIGHT:
		text_attr |= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
		break;
	case UNDERLINE:
		text_attr |= COMMON_LVB_UNDERSCORE;
		break;
	case REVERESE:
		text_attr |= COMMON_LVB_REVERSE_VIDEO;
		break;
	case HIDDEN:
		text_attr = background | (background << 4);
		break;
	}

	DWORD const length = static_cast<DWORD>(strlen(line));
	DWORD written;
	::SetConsoleTextAttribute(console, text_attr);
	::WriteConsoleA(console, line, length, &written, NULL);
	::WriteConsoleA(console, "\n", 1, &written, NULL);
	::SetConsoleTextAttribute(console, state.attributes());
}

std::wstring get_title()
{
	std::wstring title(1024, 0);

	DWORD length = ::GetConsoleTitleW(&title[0], static_cast<DWORD>(title.size()));
	if (length >= title.size())
	{
		title.resize(length);
		length = ::GetConsoleTitleW(&title[0], static_cast<DWORD>(title.size()));
	}
	title.resize(length);

	return title;
}

void set_title(std::wstring const& title)
{
	::SetConsoleTitleW(title.c_str());
}

static void colors(HANDLE console, std::vector<short> args)
{
	if (args.empty()) args.push_back(0);

	console_state& state = console_state::current_state(console);

	for (size_t i = 0; i < args.size(); ++i)
	{
		switch (args[i])
		{
		case 0:  state = console_state::default_state(console); break;
		case 39: state.foreground = console_state::default_state(console).foreground; break;
		case 49: state.background = console_state::default_state(console).background;  break;
		case 1:  state.bold = true; break;
		case 5:  break; // blink
		case 4:  state.underline = true; break;
		case 7:  state.rvideo = true; break;
		case 8:  state.concealed = true; break;
		case 21: break; // double underline
		case 22: state.bold = false; break;
		case 24: state.underline = false; break;
		case 27: state.rvideo = false; break;
		case 28: state.concealed = false; break;
		default:
			if (30 <= args[i] && args[i] <= 37)
			{
				state.foreground = console_state::ansi_color(args[i] - 30);
			}
			else if (40 <= args[i] && args[i] <= 47)
			{
				state.background = console_state::ansi_color(args[i] - 40);
			}
			break;
		}
	}

	::SetConsoleTextAttribute(console, state.attributes());
}

static void erase_chars(HANDLE console, int mode, int count = 0)
{
	COORD position;
	DWORD length, written;

	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	switch (mode)
	{
	case 0:
		// from cursor to end of display
		position = info.dwCursorPosition;
		length = count? count :
			(info.dwSize.Y - info.dwCursorPosition.Y - 1) * info.dwSize.X
				+ info.dwSize.X - info.dwCursorPosition.X - 1;
		break;
	case 1:
		// from start to cursor
		position.X = position.Y = 0;
		length = info.dwCursorPosition.Y * info.dwSize.X
			+ info.dwCursorPosition.X + 1;
		break;
	case 2:
		// clear screen and home cursor
		position.X = position.Y = 0;
		length = info.dwSize.Y * info.dwSize.X;
		break;
	default:
		return;
	}
	FillConsoleOutputCharacterW(console, L' ', length, position, &written);
	FillConsoleOutputAttribute(console, info.wAttributes, length, position, &written);
}

static void erase_lines(HANDLE console, int mode)
{
	COORD position;
	DWORD length, written;

	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	switch (mode)
	{
	case 0:
		// from cursor to end of line
		position = info.dwCursorPosition;
		length = info.dwSize.X - info.dwCursorPosition.X + 1;
		break;
	case 1:
		// from start to cursor
		position.X = 0;
		position.Y = info.dwCursorPosition.Y;
		length = info.dwCursorPosition.X + 1;
		break;
	case 2:
		// clear whole line
		position.X = 0;
		position.Y = info.dwCursorPosition.Y;
		length = info.dwSize.X;
		break;
	default:
		return;
	}
	FillConsoleOutputCharacterW(console, L' ', length, position, &written);
	FillConsoleOutputAttribute(console, info.wAttributes, length, position, &written);
}

static void insert_lines(HANDLE console, short count)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	SMALL_RECT scroll_rect;
	scroll_rect.Left = 0;
	scroll_rect.Top = info.dwCursorPosition.Y;
	scroll_rect.Right = info.dwSize.X - 1;
	scroll_rect.Bottom = info.dwSize.Y - 1;

	COORD position;
	position.X = 0;
	position.Y = info.dwCursorPosition.Y + count;

	CHAR_INFO char_info;
	char_info.Attributes = info.wAttributes;
	char_info.Char.UnicodeChar = ' ';

	::ScrollConsoleScreenBufferW(console, &scroll_rect, nullptr, position, &char_info);
}

static void delete_lines(HANDLE console, short count)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	if (count > info.dwSize.Y - info.dwCursorPosition.Y)
	{
		count = info.dwSize.Y - info.dwCursorPosition.Y;
	}

	SMALL_RECT scroll_rect;
	scroll_rect.Left = 0;
	scroll_rect.Top = info.dwCursorPosition.Y + count;
	scroll_rect.Right = info.dwSize.X - 1;
	scroll_rect.Bottom = info.dwSize.Y - 1;

	COORD position;
	position.X = 0;
	position.Y = info.dwCursorPosition.Y;

	CHAR_INFO char_info;
	char_info.Attributes = info.wAttributes;
	char_info.Char.UnicodeChar = ' ';

	::ScrollConsoleScreenBufferW(console, &scroll_rect, nullptr, position, &char_info);
}

static void delete_chars(HANDLE console, short count)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	if (info.dwCursorPosition.X + count > info.dwSize.X - 1)
	{
		count = info.dwSize.X - info.dwCursorPosition.X;
	}

	SMALL_RECT scroll_rect;
	scroll_rect.Left = info.dwCursorPosition.X + count;
	scroll_rect.Top = info.dwCursorPosition.Y;
	scroll_rect.Right = info.dwSize.X - 1;
	scroll_rect.Bottom = info.dwCursorPosition.Y;

	COORD position = info.dwCursorPosition;

	CHAR_INFO char_info;
	char_info.Attributes = info.wAttributes;
	char_info.Char.UnicodeChar = ' ';

	::ScrollConsoleScreenBufferW(console, &scroll_rect, nullptr, position, &char_info);
}

static void insert_chars(HANDLE console, short count)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	if (info.dwCursorPosition.X + count > info.dwSize.X - 1)
	{
		count = info.dwSize.X - info.dwCursorPosition.X;
	}

	SMALL_RECT scroll_rect;
	scroll_rect.Left = info.dwCursorPosition.X;
	scroll_rect.Top = info.dwCursorPosition.Y;
	scroll_rect.Right = info.dwSize.X - 1 - count;
	scroll_rect.Bottom = info.dwCursorPosition.Y;

	COORD position = info.dwCursorPosition;

	CHAR_INFO char_info;
	char_info.Attributes = info.wAttributes;
	char_info.Char.UnicodeChar = ' ';

	::ScrollConsoleScreenBufferW(console, &scroll_rect, nullptr, position, &char_info);
}

static void move_cursor(HANDLE console, short dx, short dy, bool home = false)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	COORD position;

	position.X = (home? 0 : info.dwCursorPosition.X + dx);
	if (position.X < 0) position.X = 0;
	if (position.X >= info.dwSize.X) position.X = info.dwSize.X - 1;

	position.Y = info.dwCursorPosition.Y + dy;
	if (position.Y < 0) position.Y = 0;
	if (position.Y >= info.dwSize.Y) position.Y = info.dwSize.Y - 1;

	::SetConsoleCursorPosition(console, position);
}

static void set_cursor(HANDLE console, short x, short y)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	COORD position;

	position.X = (x > 0? x - 1: info.dwCursorPosition.X);
	if (position.X < 0) position.X = 0;
	if (position.X >= info.dwSize.X) position.X = info.dwSize.X - 1;

	position.Y = (y > 0? y - 1: info.dwCursorPosition.Y);
	if (position.Y < 0) position.Y = 0;
	if (position.Y >= info.dwSize.Y) position.Y = info.dwSize.Y - 1;

	::SetConsoleCursorPosition(console, position);
}

static void show_cursor(HANDLE console, bool show)
{
	CONSOLE_CURSOR_INFO cursor_info;
	::GetConsoleCursorInfo(console, &cursor_info);
	cursor_info.bVisible = show;
	::SetConsoleCursorInfo(console, &cursor_info);
}

static void save_cursor_pos(HANDLE console)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	console_state& state = console_state::current_state(console);
	state.cursor = info.dwCursorPosition;
}

static void restore_cursor_pos(HANDLE console)
{
	console_state const& state = console_state::current_state(console);
	::SetConsoleCursorPosition(console, state.cursor);
}

static void send_esc_seq(wchar_t const* str)
{
	HANDLE console = GetStdHandle(STD_INPUT_HANDLE);

	INPUT_RECORD inp = {};
	inp.EventType = KEY_EVENT;
	inp.Event.KeyEvent.bKeyDown = true;
	inp.Event.KeyEvent.wRepeatCount = 1;
	for (; str && *str; ++str)
	{
		inp.Event.KeyEvent.uChar.UnicodeChar = *str;

		DWORD written;
		::WriteConsoleInputW(console, &inp, 1, &written);
	}
}

static void send_status(HANDLE console, int mode)
{
	(void)mode;
	CONSOLE_SCREEN_BUFFER_INFO info;
	::GetConsoleScreenBufferInfo(console, &info);

	wchar_t buf[100];
	wsprintfW(buf, L"\33[%d;%dR", info.dwCursorPosition.Y + 1, info.dwCursorPosition.X + 1);
	send_esc_seq(buf);
}

static void send_title()
{
	std::wstring title = get_title();
	title.insert(0, L"\33]1");
	title.append(L"\33\\");

	send_esc_seq(title.c_str());
}

struct esc_sequence
{
	wchar_t prefix[2], suffix;

	wchar_t const* arg_start;

	std::vector<short> args;

	wchar_t const* pt_start;
	size_t pt_len;

	void reset(wchar_t new_prefix)
	{
		prefix[0] = new_prefix;
		prefix[1] = suffix = 0;
		args.clear();
		pt_len = 0;
	}

	void add_arg(wchar_t const* end)
	{
		args.push_back(static_cast<short>(wcstol(arg_start, (wchar_t**)&end, 10)));
	}

	void interpret(HANDLE console);
};

void esc_sequence::interpret(HANDLE console)
{
	switch (prefix[0])
	{
	case '[':
		switch (prefix[1])
		{
		case 0:
			switch (suffix)
			{
			case 'm':
				return colors(console, args);
			case 'J':
				return erase_chars(console, args.empty()? 0 :args[0]);
			case 'K':
				return erase_lines(console, args.empty()? 0 :args[0]);
			case 'X':
				return erase_chars(console, 0, args.empty()? 1 : args[0]);
			case 'L':
				return insert_lines(console, args.empty()? 1 : args[0]);
			case 'M':
				return delete_lines(console, args.empty()? 1 : args[0]);
			case 'P':
				return delete_chars(console, args.empty()? 1 : args[0]);
			case '@':
				return insert_chars(console, args.empty()? 1 : args[0]);
			case 'k': case 'A':
				return move_cursor(console, 0, -(args.empty()? 1 :args[0]));
			case 'e': case 'B':
				return move_cursor(console, 0, +(args.empty()? 1 :args[0]));
			case 'a': case 'C':
				return move_cursor(console, +(args.empty()? 1 :args[0]), 0);
			case 'j': case 'D':
				return move_cursor(console, -(args.empty()? 1 :args[0]), 0);
			case 'E':
				return move_cursor(console, 0, +(args.empty()? 1 :args[0]), true);
			case 'F':
				return move_cursor(console, 0, -(args.empty()? 1 :args[0]), true);
			case '`': case 'G':
				return set_cursor(console, (args.empty()? 1 : args[0]), 0);
			case 'd':
				return set_cursor(console, 0, (args.empty()? 1 : args[0]));
			case 'f': case 'H':
				return set_cursor(console, args.size() < 2? 1 : args[1], args.size() < 1? 1 : args[0]);
			case 's':
				return save_cursor_pos(console);
			case 'u':
				return restore_cursor_pos(console);
			case 'n':
				return send_status(console, args.empty()? 0 : args[0]);
			case 't':
				if (args.size() == 1 && args[0] == 21)
				{
					send_title();
				}
				return;
			}
			break;
		case '?':
			if ((suffix == 'h' || suffix == 'l')
				&& args.size() == 1 && args[0] == 25)
			{
				show_cursor(console, suffix == 'h');
			}
			break;
		}
		break;
	case ']':
		if (prefix[1] == 0 && args.empty())
		{
			std::wstring const title(pt_start, pt_len);
			set_title(title);
		}
		break;
	}
}

void vt100_write(wchar_t const* str, HANDLE console)
{
	DWORD written, buf_len = 0;

	enum esc_parser_state { START, ESC, PREFIX, ARG, SUFFIX } state = START;
	esc_sequence esc_seq;

	// loop over str, find ESC sequence, interpret it
	for (; str && *str; ++str)
	{
		switch (state)
		{
		case START:
			if (*str == 0x1b)
			{
				if (buf_len)
				{
					::WriteConsoleW(console, str - buf_len, buf_len, &written, nullptr);
					buf_len = 0;
				}
				state = ESC;
			}
			else
			{
				++buf_len;
			}
			break;

		case ESC:
			switch (*str)
			{
			case 0x1b:
				continue;
			case '[': case ']':
				state = PREFIX;
				esc_seq.reset(*str);
				break;
			default:
				state = START;
				break;
			}
			break;

		case PREFIX:
			switch (*str)
			{
			case ';':
				state = SUFFIX;
				break;
			case '?': case '>':
				esc_seq.prefix[1] = *str;
				break;
			default:
				if (isdigit(*str))
				{
					state = ARG;
					esc_seq.arg_start = str;
				}
				else
				{
					esc_seq.args.clear();
					esc_seq.suffix = *str;
					esc_seq.interpret(console);
					state = START;
				}
				break;
			}
			break;

		case ARG:
			if (*str == ';')
			{
				esc_seq.add_arg(str);
				state = (esc_seq.prefix[0] == ']'? SUFFIX : PREFIX);
			}
			else if (!isdigit(*str))
			{
				esc_seq.add_arg(str);
				esc_seq.suffix = *str;
				esc_seq.interpret(console);
				state =START;
			}
			break;

		case SUFFIX:
			switch (*str)
			{
			case 0x07: case 0x1b:
				esc_seq.interpret(console);
				state = START;
				break;
			default:
				++esc_seq.pt_len;
				break;
			}
			break;

		default:
			state = START;
			break;
		}
	}

	// write remaining string
	if (buf_len)
	{
		::WriteConsoleW(console, str - buf_len, buf_len, &written, nullptr);
		buf_len = 0;
	}
}

template<int ShowCmd>
static void show_console_window()
{
	ShowWindow(GetConsoleWindow(), ShowCmd);
}

#else

void println(char const* line, color fg, color bg, attribute attr)
{
	printf("\33[%d;%d;%dm%s\n\33[0m", attr, fg + 30, bg + 40, line);
}

static std::string process_title;

std::string get_title()
{
	if (process_title.empty())
	{
		process_title = runtime::instance(v8::Isolate::GetCurrent()).args()[0];
	}
	return process_title;
}

void set_title(std::string const& title)
{
	process_title = title;
	printf("\33]0;%s\a", title.c_str());
}


#endif

template<int fd>
static void console_read(v8::FunctionCallbackInfo<v8::Value> const&)
{
}

template<int fd>
void console_write(v8::FunctionCallbackInfo<v8::Value> const& args)
{
#if OS(WINDOWS)
	HANDLE handle[] = { GetStdHandle(STD_INPUT_HANDLE), GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE) };
#else
	FILE* stream[] = { stdin, stdout, stderr };
#endif

	v8::HandleScope scope(args.GetIsolate());
	for (int i = 0, len = args.Length(); i < len; ++i)
	{
		v8::Handle<v8::String> str =args[i]->ToString();
#if OS(WINDOWS)
		v8::String::Value utf16(str);
		console::vt100_write(reinterpret_cast<wchar_t const*>(*utf16), handle[fd]);
#else
		v8::String::Utf8Value utf8(str);
		fputs(*utf8, stream[fd]);
#endif
	}
}

template<int fd>
static v8::Handle<v8::Value> create_stdio_stream(v8::Isolate* isolate)
{
	v8pp::module stream_template(isolate);
	switch (fd)
	{
	case 0:
		stream_template.set("read", &console_read<fd>);
		break;
	case 1:
		stream_template.set("write", &console_write<fd>);
		break;
	case 2:
		stream_template.set("write", &console_write<fd>);
		break;
	}

	return stream_template.new_instance();
}

void setup_bindings(v8pp::module& bindings)
{
	v8::Isolate* isolate = bindings.isolate();

	v8pp::module console_template(isolate);

	/**
	@module console
	**/
	console_template
		.set_const("stdin",  create_stdio_stream<0>(isolate))
		.set_const("stdout", create_stdio_stream<1>(isolate))
		.set_const("stderr", create_stdio_stream<2>(isolate))
		/**
		@property caption {String}
		String property to get/set console window caption.
		**/
		.set("caption", v8pp::property(get_title, set_title))
#if OS(WINDOWS)
		/**
		@function minimize()
		Minimize console window, Windows only.
		**/
		.set("minimize", show_console_window<SW_MINIMIZE>)
		/**
		@function restore()
		Restore minimized console window, Windows only.
		**/
		.set("restore", show_console_window<SW_RESTORE>)
		/**
		@function hide()
		Hide console window, Windows only.
		**/
		.set("hide", show_console_window<SW_HIDE>)
		/**
		@function show()
		Show hidden console window, Windows only.
		**/
		.set("show", show_console_window<SW_SHOW>)
#endif
		;

	bindings.set("console", console_template);
}

}} // aspect::console
