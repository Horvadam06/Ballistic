#include "input_poll.h"

#ifdef _WIN32
#include <conio.h>

int input_available(void)
{
    return _kbhit();
}

int input_getchar(void)
{
    if (_kbhit())
        return _getch();
    return -1;
}

void input_init(void) {}
void input_shutdown(void) {}
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

static struct termios orig_term;

void input_init(void)
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_term);
    raw = orig_term;

    raw.c_lflag &= ~(ICANON | ECHO); // no line buffering, no echo
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void input_shutdown(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

int input_available(void)
{
    struct timeval tv = {0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int input_getchar(void)
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1)
        return c;
    return -1;
}

#endif
