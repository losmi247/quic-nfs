#include "mount_list.h"

/*
* Creates a new MountList entry with the given client_hostname and for the directory being mounted
* given in 'directory_absolute_path', and ith nextentry equal to NULL.
*
* The user of this function takes the responsiblity to free the mount_name.name, dirpath.path,
* mount_name, dirpath, and the MountList itself. This should be done at server shutdown, using
* the 'clean_up_mount_list' function to deallocate the entire mount list.
*/
Mount__MountList *create_new_mount_list_entry(char *client_hostname, char *directory_absolute_path) {
    Mount__MountList *mount_list_entry = malloc(sizeof(Mount__MountList));
    mount__mount_list__init(mount_list_entry);

    Mount__Name *mount_name = malloc(sizeof(Mount__Name));
    mount__name__init(mount_name);
    mount_name->name = strdup(client_hostname);

    Mount__DirPath *dirpath = malloc(sizeof(Mount__DirPath));
    mount__dir_path__init(dirpath);
    dirpath->path = strdup(directory_absolute_path);

    mount_list_entry->hostname = mount_name;
    mount_list_entry->directory = dirpath;
    mount_list_entry->nextentry = NULL;

    return mount_list_entry;
}

/*
* Creates a new MountList entry with the given client_hostname and for the directory being mounted
* given in 'directory_absolute_path', and adds it to the head of the MountList given in 'current_list_head'.
*
* The user of this function takes the responsiblity to free the mount_name.name, dirpath.path,
* mount_name, dirpath, and the MountList itself, for the mount list entry created in this function.
* This should be done at server shutdown, using the 'clean_up_mount_list' function to deallocate the entire mount list.
*/
void add_mount_list_entry(char *client_hostname, char *directory_absolute_path, Mount__MountList **current_list_head) {
    Mount__MountList *new_mount_list_entry = create_new_mount_list_entry(client_hostname, directory_absolute_path);

    new_mount_list_entry->nextentry = *current_list_head;
    *current_list_head = new_mount_list_entry;
}

void free_mount_list_entry(Mount__MountList *mount_list_entry) {
    free(mount_list_entry->hostname->name);
    free(mount_list_entry->hostname);

    free(mount_list_entry->directory->path);
    free(mount_list_entry->directory);

    free(mount_list_entry);
}

/*
* Deallocates all heap-allocated fields in the mount list entries in the given mount list,
* as well as the mount list entries themselves.
*
* This funtion should be used at server shutdown to clean up the mount list.
*/
void clean_up_mount_list(Mount__MountList *mount_list_head) {
    while(mount_list_head != NULL) {
        Mount__MountList *next = mount_list_head->nextentry;

        free_mount_list_entry(mount_list_head);

        mount_list_head = next;
    }
}