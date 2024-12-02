#ifndef inode_cache__header__INCLUDED
#define inode_cache__header__INCLUDED

#include <sys/types.h> 
#include <stdlib.h>
#include <string.h>

struct InodeCacheMapping {
    ino_t inode_number;
    char *absolute_path;
    struct InodeCacheMapping *next;
};
typedef struct InodeCacheMapping *InodeCache;

void add_inode_mapping(ino_t inode_number, char *absolute_path, InodeCache *head);

int remove_inode_mapping(ino_t inode_number, InodeCache *head);

char *get_absolute_path_from_inode_number(ino_t inode_number, InodeCache head);

void clean_up_inode_cache(InodeCache inode_cache);

#endif /* inode_cache__header__INCLUDED */