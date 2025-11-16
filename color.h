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

//reg add HKCU\\Console /v VirtualTerminalLevel /t REG_DWORD /d 1 /f
//make it print out if no colours

typedef enum {CM_MONO, CM_COLOUR} ColorMode;

ColorMode get_color_mode(void);
void set_color_mode(ColorMode mode);

HRESULT print_colored(const char *color, const char *string, bool always_p);

#endif