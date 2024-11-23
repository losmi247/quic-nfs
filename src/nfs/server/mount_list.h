#ifndef mount_list__header__INCLUDED
#define mount_list__header__INCLUDED

#include "src/serialization/mount/mount.pb-c.h"

Mount__MountList *create_new_mount_entry(char *name, Mount__DirPath dirpath);

void add_mount_entry(Mount__MountList **current_list_head, Mount__MountList *new_mount_entry);

void clean_up_mount_list(Mount__MountList *list_head);

#endif /* mount_list__header__INCLUDED */