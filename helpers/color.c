#include "color.h"
#include <stdio.h>
#include <stdarg.h>

void change_colour(const char *color)
{
    printf(color);
}

void Error_message(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    printf(COLOR_ERROR "[Error] ");
    vprintf(format, ap);
    printf(COLOR_RESET);
}

void Warning_message(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    printf(COLOR_WARNING "[Warning] ");
    vprintf(format, ap);
    printf(COLOR_RESET);
}