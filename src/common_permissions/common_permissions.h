#ifndef common_permissions__header__INCLUDED
#define common_permissions__header__INCLUDED

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

#include "src/error_handling/error_handling.h"

int check_root_user(uid_t uid, gid_t gid);

int check_read_permission(char *absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_write_permission(char *absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_execute_permission(char *absolute_path, uid_t caller_uid, gid_t caller_gid);

#endif /* common_permissions__header__INCLUDED */