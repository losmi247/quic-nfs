#include "error_handling.h"

/*
* Equips perror() with ability to print a formatted message.
*/
void perror_msg(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char msg[PERROR_BUF_SIZE];
    vsnprintf(msg, PERROR_BUF_SIZE, format, args);

    perror(msg);

    va_end(args);
}