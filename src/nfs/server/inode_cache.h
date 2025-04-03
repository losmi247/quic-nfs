#ifndef inode_cache__header__INCLUDED
#define inode_cache__header__INCLUDED

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "src/serialization/nfs_fh/nfs_fh.pb-c.h"

struct InodeCacheMapping {
    NfsFh__NfsFileHandle *nfs_filehandle;
    char *absolute_path;
    struct InodeCacheMapping *next;
};
typedef struct InodeCacheMapping *InodeCache;

int add_inode_mapping(NfsFh__NfsFileHandle *nfs_filehandle, char *absolute_path, InodeCache *head);

int remove_inode_mapping_by_inode_number(ino_t inode_number, InodeCache *head);

int remove_inode_mapping_by_absolute_path(char *absolute_path, InodeCache *head);

int update_inode_mapping_absolute_path_by_absolute_path(char *absolute_path, char *new_absolute_path, InodeCache *head);

char *get_absolute_path_from_inode_number(ino_t inode_number, InodeCache head);

NfsFh__NfsFileHandle *get_nfs_filehandle_from_inode_number(ino_t inode_number, InodeCache head);

void clean_up_inode_cache(InodeCache inode_cache);

#endif /* inode_cache__header__INCLUDED */