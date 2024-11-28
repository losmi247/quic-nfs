#ifndef error_handling__header__INCLUDED
#define error_handling__header__INCLUDED

#define PERROR_BUF_SIZE 256

#include <stdarg.h>
#include <stdio.h>

void perror_msg(const char *format, ...);

#endif /* error_handling__header__INCLUDED */