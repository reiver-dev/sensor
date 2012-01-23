#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "debug.h"

int rvr_debug = 3;

void debug(int level, const char *function, char *format, ...){
	if (rvr_debug >= level) return;
	char line[512];
	time_t now;
	time(&now);
	strftime(line, sizeof(line), "(%d:%m:%Y -- %T)", localtime(&now));
	va_list arg;
	va_start(arg, format);
	sprintf(line+strlen(line), "[%u,%s]", getpid(), function);
	vsprintf(line+strlen(line), format, arg);
	va_end(arg);
	printf(line);
}
