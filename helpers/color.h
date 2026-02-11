#ifndef COLOR_H
#define COLOR_H

#include "debug.h"

/* Reset */
#define ANSI_RESET "\x1b[0m"

/* Standard colors */
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_WHITE "\x1b[37m"

/* Bright colors */
#define ANSI_BRIGHT_RED "\x1b[91m"
#define ANSI_BRIGHT_GREEN "\x1b[92m"
#define ANSI_BRIGHT_YELLOW "\x1b[93m"
#define ANSI_BRIGHT_BLUE "\x1b[94m"
#define ANSI_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_BRIGHT_CYAN "\x1b[96m"
#define ANSI_BRIGHT_WHITE "\x1b[97m"

/* Grayscale / emphasis */
#define ANSI_DIM "\x1b[2m"
#define ANSI_GRAY "\x1b[90m"

/* Text styles (use sparingly) */
#define ANSI_BOLD "\x1b[1m"
#define ANSI_UNDERLINE "\x1b[4m"

/* UI structure */
#define COLOR_TITLE "\x1b[2;37m" // dim white
#define COLOR_HINT "\x1b[2;90m"
#define COLOR_PROMPT "\x1b[2;37m"

/* Commands & input */
#define COLOR_COMMAND "\x1b[36m"
#define COLOR_PARAM "\x1b[33m"

/* Status */
#define COLOR_SUCCESS "\x1b[32m"
#define COLOR_ERROR "\x1b[31m"
#define COLOR_WARNING "\x1b[33m"
#define COLOR_INFO "\x1b[36m"

/* w char's */
#define HLINE "―"
#define VLINE "│"

#define CURSOR_HIDE  "\033[?25l"
#define CURSOR_SHOW  "\033[?25h"


/* Reset alias */
#define COLOR_RESET ANSI_RESET

#define ASCI_ANGEL                          \
  "                      .              \n" \
  "                     .           .   \n" \
  "                    ..        .:;.   \n" \
  "                   .:...    :;+;:.   \n" \
  "                   ;:::   .++;;:     \n" \
  "                  ;;::.  +;;;:::     \n" \
  "                  +;::::;:;;;::.     \n" \
  "                   :;;;;;:::::.      \n" \
  "              .&&Xx++;:+x:;::..      \n" \
  "               :&Xxx+xXx::.:..       \n" \
  "                :$+;;;:.......       \n" \
  "              :$$++;:;.::...         \n" \
  "           .Xx;:+++;:.....           \n" \
  "         .x:    x;;:::...            \n" \
  "       .x      ;++:.:                \n" \
  "     .;       ::;... .               \n" \
  "   ;:         :+::..  :              \n" \
  "  :          :+::..                  \n" \
  "            ;:.         .            \n" \
  "            ..::                     \n" \
  "            :  ..                    \n" \
  "    ..:::   :..                      \n" \
  " XXxx++x++;;;..                 .    \n" \
  " &&$$Xx++++;;;:::.             ....  \n" \
  " :+X$Xxxxx+++;;+;;:.          ..     \n" \
  " .:.X$XxXx+x++;;;;::....    .        \n"

/*#ifdef NO_COLOR
  #define COLOR_COMMAND ""
  #define COLOR_SUCCESS ""
  #define COLOR_ERROR   ""
  #define COLOR_RESET   ""
#endif*/

typedef enum
{
  CM_MONO = 0,
  CM_COLOUR = 1
} ColorMode;

ColorMode get_color_mode(void);
void set_color_mode(ColorMode mode);

void Error_message(const char *format, ...);
void Warning_message(const char *format, ...);

void change_colour(const char *color);

#endif