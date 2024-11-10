#ifndef file_management__header__INCLUDED
#define file_management__header__INCLUDED

#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()
#include <sys/stat.h> // fstat()
#include <fcntl.h>

int get_inode_number(char *file_or_directory_absolute_path, ino_t *inode_number);

#endif /* file_management__header__INCLUDED */