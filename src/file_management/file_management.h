#ifndef file_management__header__INCLUDED
#define file_management__header__INCLUDED

#define _GNU_SOURCE // so that we can use name_to_handle_at and open_by_handle_at Linux-specific syscalls

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()
#include <sys/stat.h> // fstat()
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "src/serialization/nfs/nfs.pb-c.h"

int get_inode_number(char *absolute_path, ino_t *inode_number);

int get_attributes(char *absolute_path, Nfs__FAttr *fattr);

#endif /* file_management__header__INCLUDED */