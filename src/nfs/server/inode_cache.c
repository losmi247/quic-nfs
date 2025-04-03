#include "inode_cache.h"

#include "src/filehandle_management/filehandle_management.h"

#include <stdio.h>

/*
 * Frees heap allocated space in the given InodeCacheMapping and the InodeCacheMapping itself.
 *
 * Does nothing if the given InodeCacheMapping is null.
 */
void free_inode_cache_mapping(struct InodeCacheMapping *inode_cache_mapping) {
    if (inode_cache_mapping == NULL) {
        return;
    }

    free(inode_cache_mapping->nfs_filehandle);
    free(inode_cache_mapping->absolute_path);
    free(inode_cache_mapping);
}

/*
 * Creates a new inode cache entry mapping the given NFS filehandle to the given absolute path
 * and adds it to the head of the given inode cache 'head'.
 *
 * The user of this function takes the responsiblity to free the absolute_path in the created mapping
 * as well as the mapping itself.
 * This should be done at server shutdown, using the 'clean_up_inode_cache' function to deallocate the
 * entire inode cache.
 *
 * Returns 0 on success and > 0 on failure.
 */
int add_inode_mapping(NfsFh__NfsFileHandle *nfs_filehandle, char *absolute_path, InodeCache *head) {
    if (nfs_filehandle == NULL) {
        return 1;
    }

    InodeCache new_mapping = malloc(sizeof(struct InodeCacheMapping));
    if (new_mapping == NULL) {
        return 1;
    }

    new_mapping->nfs_filehandle = malloc(sizeof(NfsFh__NfsFileHandle));
    if (new_mapping->nfs_filehandle == NULL) {
        return 1;
    }
    *(new_mapping->nfs_filehandle) = deep_copy_nfs_filehandle(nfs_filehandle);

    new_mapping->absolute_path = strdup(absolute_path);

    new_mapping->next = *head;

    *head = new_mapping;

    return 0;
}

/*
 * Removes an entry with the given inode number from the inode cache,
 * and deallocates that removed InodeCacheMapping along with the
 * fields inside it.
 *
 * Returns 0 on successful removal of the entry, 1 if the corresponding entry
 * could not be found, and > 1 on failure otherwise.
 */
int remove_inode_mapping_by_inode_number(ino_t inode_number, InodeCache *head) {
    if (head == NULL) {
        return 2;
    }

    // the entry we want to remove is the first in the list
    if ((*head)->nfs_filehandle->inode_number == inode_number) {
        struct InodeCacheMapping *new_head = (*head)->next;

        free_inode_cache_mapping(*head);

        *head = new_head;

        return 0;
    }

    // the entry we want to remove is somewhere in the middle of the list
    struct InodeCacheMapping *curr_mapping = (*head)->next, *prev_mapping = *head;
    while (curr_mapping != NULL) {
        if (curr_mapping->nfs_filehandle->inode_number == inode_number) {
            prev_mapping->next = curr_mapping->next;

            free_inode_cache_mapping(curr_mapping);

            return 0;
        }

        prev_mapping = curr_mapping;
        curr_mapping = curr_mapping->next;
    }

    return 1;
}

/*
 * Removes an entry with the given absolute path from the inode cache,
 * and deallocates that removed InodeCacheMapping along with the
 * fields inside it.
 *
 * Returns 0 on successful removal of the entry, 1 if the corresponding entry
 * could not be found, and > 1 on failure otherwise.
 */
int remove_inode_mapping_by_absolute_path(char *absolute_path, InodeCache *head) {
    if (head == NULL) {
        return 2;
    }

    // the entry we want to remove is the first in the list
    if (strcmp((*head)->absolute_path, absolute_path) == 0) {
        struct InodeCacheMapping *new_head = (*head)->next;

        free_inode_cache_mapping(*head);

        *head = new_head;

        return 0;
    }

    // the entry we want to remove is somewhere in the middle of the list
    struct InodeCacheMapping *curr_mapping = (*head)->next, *prev_mapping = *head;
    while (curr_mapping != NULL) {
        if (strcmp(curr_mapping->absolute_path, absolute_path) == 0) {
            prev_mapping->next = curr_mapping->next;

            free_inode_cache_mapping(curr_mapping);

            return 0;
        }

        prev_mapping = curr_mapping;
        curr_mapping = curr_mapping->next;
    }

    return 1;
}

/*
 * Finds the entry with the given absolute path in the inode cache, frees its old absolute path
 * and updates its absolute path to the new absolute path given in 'new_absolute_path' argument
 * (a copy of this string is created on heap so that the user can free 'new_absolute_path' on their side safely).
 *
 * Returns 0 on successful update of the entry, 1 if the corresponding entry
 * could not be found, and > 1 on failure otherwise.
 */
int update_inode_mapping_absolute_path_by_absolute_path(char *absolute_path, char *new_absolute_path,
                                                        InodeCache *head) {
    if (head == NULL) {
        return 2;
    }

    struct InodeCacheMapping *curr_mapping = *head;
    while (curr_mapping != NULL) {
        if (strcmp(curr_mapping->absolute_path, absolute_path) == 0) {
            free(curr_mapping->absolute_path);

            curr_mapping->absolute_path = strdup(new_absolute_path);

            return 0;
        }

        curr_mapping = curr_mapping->next;
    }

    return 1;
}

/*
 * Retrieves the absolute path of a file/directory with the given inode number, or
 * returns NULL if the corresponding mapping could not be found in the cache.
 */
char *get_absolute_path_from_inode_number(ino_t inode_number, InodeCache head) {
    struct InodeCacheMapping *inode_cache_mapping = head;
    while (inode_cache_mapping != NULL) {
        if (inode_cache_mapping->nfs_filehandle->inode_number == inode_number) {
            return inode_cache_mapping->absolute_path;
        }

        inode_cache_mapping = inode_cache_mapping->next;
    }

    return NULL;
}

/*
 * Retrieves the NFS filehandle of a file/directory with the given inode number, or
 * returns NULL if the corresponding mapping could not be found in the cache.
 */
NfsFh__NfsFileHandle *get_nfs_filehandle_from_inode_number(ino_t inode_number, InodeCache head) {
    struct InodeCacheMapping *inode_cache_mapping = head;
    while (inode_cache_mapping != NULL) {
        if (inode_cache_mapping->nfs_filehandle->inode_number == inode_number) {
            return inode_cache_mapping->nfs_filehandle;
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
    while (inode_cache_mapping != NULL) {
        struct InodeCacheMapping *next = inode_cache_mapping->next;

        free_inode_cache_mapping(inode_cache_mapping);

        inode_cache_mapping = next;
    }
}