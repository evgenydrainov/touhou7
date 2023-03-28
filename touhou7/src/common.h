#pragma once

#include <stdio.h>

#define TH_STRINGIZE(x) TH_STRINGIZE2(x)
#define TH_STRINGIZE2(x) #x
#define TH_LINE_STRING TH_STRINGIZE(__LINE__)

#define TH_LOG_ERROR(fmt, ...) printf(__FILE__ ":" TH_LINE_STRING ":" fmt "\n", __VA_ARGS__)

#define TH_SHOW_ERROR(fmt, ...)																\
	do {																					\
		bool fullscreen = false;															\
		if (fullscreen) {																	\
			TH_LOG_ERROR(fmt, __VA_ARGS__);													\
		} else {																			\
			char* buf = new char[1000];														\
			snprintf(buf, 1000, __FILE__ ":" TH_LINE_STRING ": " fmt, __VA_ARGS__);			\
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", buf, 0);				\
			delete[] buf;																	\
		}																					\
	} while (0)
