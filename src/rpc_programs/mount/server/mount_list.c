#include <stdlib.h>

#include "src/serialization/mount/mount.pb-c.h"

#include "mount_list.h"

Mount__MountList *create_new_mount_entry(char *name, Mount__DirPath *dirpath) {
    Mount__MountList *mount_list_entry = malloc(sizeof(Mount__MountList));
    mount__mount_list__init(mount_list_entry);

    mount_list_entry->hostname = malloc(sizeof(Mount__Name));
    mount__name__init(mount_list_entry->hostname);

    mount_list_entry->hostname->name = name;
    mount_list_entry->directory = dirpath;

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

    free(mount_list_entry);
}

void cleanup_mount_list(Mount__MountList *list_head) {
    if(list_head == NULL) {
        return;
    }

    cleanup_mount_list(list_head->nextentry);
    free_mount_list_entry(list_head);
}