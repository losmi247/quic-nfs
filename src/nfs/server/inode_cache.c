#include "inode_cache.h"

/*
* Creates a new entry in the inode cache.
*/
void add_inode_mapping(ino_t inode_number, char *absolute_path, InodeCache *head) {
    InodeCache new_mapping = malloc(sizeof(struct InodeCacheMapping));
    new_mapping->inode_number = inode_number;
    new_mapping->absolute_path = absolute_path;
    new_mapping->next = *head;

    *head = new_mapping;
}

/*
* Retrieves the absolute path of a file/directory with the given inode number, or
* returns NULL if the corresponding mapping could not be found in the cache.
*/
char *get_absolute_path_from_inode_number(ino_t inode_number, InodeCache head) {
    InodeCache mapping = head;
    while(mapping != NULL) {
        if(mapping->inode_number == inode_number) {
            return mapping->absolute_path;
        }

        mapping = mapping->next;
    }

    return NULL;
}

/*
* Deallocates all inode mappings in the given inode cache.
*/
void clean_up_inode_cache(InodeCache inode_cache) {
    while(inode_cache != NULL) {
        struct InodeCacheMapping *next = inode_cache->next;

        free(inode_cache);

        inode_cache = next;
    }
}