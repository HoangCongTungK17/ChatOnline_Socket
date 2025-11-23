#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_message(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    // In thời gian hiện tại
    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[20];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(stdout, "[%s] | LOG | ", time_buffer);
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");

    va_end(args);
}