#pragma once

#ifdef _DEBUG
#include "stdio.h"
#define TRACE(a,...) do { char xfmt[255]; char files[] = __FILE__; char * p = &files[strlen(files)-1]; while (p[0] != '\\') { p--; } _snprintf_s(xfmt,254,a,__VA_ARGS__); DebugMsg(p,__LINE__,xfmt); } while (0)

void DebugMsg(const char * const file, int line, const char * const text);
#else
#define TRACE(a,...)
#endif

#define TRACEH(a,...)
