#ifndef mount_list__header__INCLUDED
#define mount_list__header__INCLUDED

#include <stdlib.h>
#include <string.h>

#include "src/serialization/mount/mount.pb-c.h"

void add_mount_list_entry(char *client_hostname, char *directory_absolute_path, Mount__MountList **current_list_head);

void clean_up_mount_list(Mount__MountList *mount_list_head);

#endif /* mount_list__header__INCLUDED */