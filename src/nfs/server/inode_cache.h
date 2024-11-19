#ifndef inode_cache__header__INCLUDED
#define inode_cache__header__INCLUDED

#include <sys/types.h> 
#include <stdlib.h>

struct InodeCacheMapping {
    ino_t inode_number;
    char *absolute_path;
    struct InodeCacheMapping *next;
};
typedef struct InodeCacheMapping *InodeCache;

void add_inode_mapping(ino_t inode_number, char *absolute_path, InodeCache *head);

char *get_absolute_path_from_inode_number(ino_t inode_number, InodeCache head);

#endif /* inode_cache__header__INCLUDED */