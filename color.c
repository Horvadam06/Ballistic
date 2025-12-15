#include "color.h"
#include "stdio.h"

// static to make it "private" to this file
static ColorMode current_mode = CM_MONO; 

// returns the colour mode choosen at config
ColorMode get_color_mode(void)
{
    return current_mode;
}

//set's the colour mode
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

//if the colour mode is coloured, print's out the text coloured, 
//else if the always_p flag is set, print's out the text normally
P_HRESULT print_colored(const char *color, const char *string, bool always_p)
{
    if (current_mode == CM_COLOUR)
    {
        printf("%s%s%s", color, string, ANSI_COLOR_RESET);
        return P_S_OK;
    }
    else if (always_p)
    {
        printf("%s", string);
    }
    return P_S_FALSE;
}
