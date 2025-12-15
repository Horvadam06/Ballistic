#ifndef INPUT_POLL_H
#define INPUT_POLL_H

int input_available(void);
int input_getchar(void);   // returns char or -1 if none

void input_init(void);
void input_shutdown(void);

#endif
