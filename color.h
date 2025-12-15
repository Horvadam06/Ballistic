#ifndef COLOR_H
#define COLOR_H

#include "debug.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef enum
{
    CM_MONO = 0,
    CM_COLOUR = 1
} ColorMode;

ColorMode get_color_mode(void);
void set_color_mode(ColorMode mode);

P_HRESULT print_colored(const char *color, const char *string, bool always_p);
void change_colour(const char *color);

#endif