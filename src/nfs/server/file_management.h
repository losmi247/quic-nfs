#ifndef file_management__header__INCLUDED
#define file_management__header__INCLUDED

#define _POSIX_C_SOURCE 200809L // to be able to use stat() in sys/stat.h
#define _DEFAULT_SOURCE         // to be able to use seekdir() and telldir() in dirent.h

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // stat()
#include <sys/types.h>
#include <time.h>
#include <unistd.h> // read(), write(), close()

#include "src/error_handling/error_handling.h"

#include "src/nfs/nfs_common.h"

#include "src/serialization/nfs/nfs.pb-c.h"

#include "inode_cache.h"

/*
 * General file management functions used by many Nfs procedures
 */

NfsFh__NfsFileHandle *create_nfs_filehandle(char *absolute_path, InodeCache *inode_number_cache);

int get_attributes(char *absolute_path, Nfs__FAttr *fattr);

void clean_up_fattr(Nfs__FAttr *fattr);

/*
 * File management functions used by NFSPROC_READ
 */

int read_from_file(char *file_absolute_path, off_t offset, size_t byte_count, uint8_t *destination_buffer,
                   size_t *bytes_read);

/*
 * File management functions used by NFSPROC_WRITE
 */

int write_to_file(char *file_absolute_path, off_t offset, size_t byte_count, uint8_t *source_buffer);

#endif /* file_management__header__INCLUDED */