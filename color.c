#include "color.h"
#include "stdio.h"

static ColorMode current_mode = CM_MONO; // static to make it "private" to this file

ColorMode get_color_mode(void)
{
    return current_mode;
}

void set_color_mode(ColorMode mode)
{
    switch (mode)
    {
    case CM_MONO:
    case CM_COLOUR:
        current_mode = mode;
        break;
    default:
        current_mode = CM_MONO;
    }
}

void print_colored(const char *color, const char *string)
{
    if (current_mode == CM_COLOUR)
        printf("%s%s%s", color, string, ANSI_COLOR_RESET);
    else
        printf("%s", string);
}
