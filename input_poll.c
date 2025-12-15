#include "input_poll.h"

#ifdef _WIN32
#include <conio.h>
int input_available(void)
{
    return _kbhit();
}
#else
#include <sys/select.h>
#include <unistd.h>

int input_available(void)
{
    struct timeval tv = {0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}
#endif
