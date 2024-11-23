#include <stdlib.h>

#include "src/serialization/mount/mount.pb-c.h"

#include "mount_list.h"

Mount__MountList *create_new_mount_entry(char *name, Mount__DirPath dirpath) {
    Mount__MountList *mount_list_entry = malloc(sizeof(Mount__MountList));
    mount__mount_list__init(mount_list_entry);

    mount_list_entry->hostname = malloc(sizeof(Mount__Name));
    mount__name__init(mount_list_entry->hostname);

    mount_list_entry->hostname->name = name;
    mount_list_entry->directory = malloc(sizeof(Mount__DirPath));
    mount__dir_path__init(mount_list_entry->directory);
    *(mount_list_entry->directory) = dirpath;

    return mount_list_entry;
}

void add_mount_entry(Mount__MountList **current_list_head, Mount__MountList *new_mount_entry) {
    new_mount_entry->nextentry = *current_list_head;
    *current_list_head = new_mount_entry;
}

void free_mount_list_entry(Mount__MountList *mount_list_entry) {
    free(mount_list_entry->hostname->name);
    free(mount_list_entry->hostname);

    mount__dir_path__free_unpacked(mount_list_entry->directory, NULL);

    // don't need to free(mount_list_entry->directory) even though it was allocated in create_new_mount_entry - free_unpacked takes care of it

    free(mount_list_entry);
}

void clean_up_mount_list(Mount__MountList *list_head) {
    while(list_head != NULL) {
        Mount__MountList *next = list_head->nextentry;

        free_mount_list_entry(list_head);

        list_head = next;
    }
}