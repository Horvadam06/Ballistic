#include "terminal_info.h"
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>

void terminal_size(int *x, int *y)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void set_terminal_margins(int left, int top, int right, int bottom)
{
    // CSI Pn ; Pn ; Pn ; Pn T
    // Set left/right margins: CSI Pn ; Pn s
    // Set top/bottom margins: CSI Pn ; Pn r

    printf("\033[%d;%ds", top, bottom); // Vertical margins
    printf("\033[%d;%dr", left, right); // Horizontal margins
}

#else
void terminal_size(int *x, int *y)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *x = w.ws_col;
    *y = w.ws_row;
}

#endif