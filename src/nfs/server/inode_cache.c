#include "inode_cache.h"

void free_inode_cache_mapping(struct InodeCacheMapping *inode_cache_mapping) {
    free(inode_cache_mapping->absolute_path);
    free(inode_cache_mapping);
}

/*
* Creates a new inode cahce entry mapping the given inode_number to the given absolute path
* and adds it to the head of the given inode cache 'head'.
*
* The user of this function takes the responsiblity to free the absolute_path in the created mapping
* as well as the mapping itself.
* This should be done at server shutdown, using the 'clean_up_inode_cache' function to deallocate the 
* entire inode cache.
*/
void add_inode_mapping(ino_t inode_number, char *absolute_path, InodeCache *head) {
    InodeCache new_mapping = malloc(sizeof(struct InodeCacheMapping));
    new_mapping->inode_number = inode_number;
    new_mapping->absolute_path = strdup(absolute_path);
    new_mapping->next = *head;

    *head = new_mapping;
}

/*
* Removes an entry with the given inode number from the inode cache,
* and deallocates that removed InodeCacheMapping along with the 
* absolute path inside it.
*
* Returns 0 on successful removal of the entry, and > 0 otherwise.
*/
int remove_inode_mapping(ino_t inode_number, InodeCache *head) {
    if(head == NULL) {
        return 1;
    }

    // the entry we want to remove is the first in the list
    if((*head)->inode_number == inode_number) {
        struct InodeCacheMapping *new_head = (*head)->next;

        free_inode_cache_mapping(*head);

        *head = new_head;

        return 0;
    }

    // the entry we want to remove is somewhere in the middle of the list
    struct InodeCacheMapping *curr_mapping = (*head)->next, *prev_mapping = *head;
    while(curr_mapping != NULL) {
        if(curr_mapping->inode_number == inode_number) {
            prev_mapping->next = curr_mapping->next;

            free_inode_cache_mapping(curr_mapping);

            return 0;
        }

        prev_mapping = curr_mapping;
        curr_mapping = curr_mapping->next;
    }

    return 2;
}

/*
* Retrieves the absolute path of a file/directory with the given inode number, or
* returns NULL if the corresponding mapping could not be found in the cache.
*/
char *get_absolute_path_from_inode_number(ino_t inode_number, InodeCache head) {
    struct InodeCacheMapping *inode_cache_mapping = head;
    while(inode_cache_mapping != NULL) {
        if(inode_cache_mapping->inode_number == inode_number) {
            return inode_cache_mapping->absolute_path;
        }

        inode_cache_mapping = inode_cache_mapping->next;
    }

    return NULL;
}

/*
* Deallocates all inode mappings and absolute paths in them, in the given inode cache.
*/
void clean_up_inode_cache(InodeCache inode_cache) {
    struct InodeCacheMapping *inode_cache_mapping = inode_cache;
    while(inode_cache_mapping != NULL) {
        struct InodeCacheMapping *next = inode_cache_mapping->next;

        free_inode_cache_mapping(inode_cache_mapping);

        inode_cache_mapping = next;
    }
}