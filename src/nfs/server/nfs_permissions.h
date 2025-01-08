#ifndef nfs_permissions__header__INCLUDED
#define nfs_permissions__header__INCLUDED

#include "src/common_permissions/common_permissions.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

#include "src/serialization/nfs/nfs.pb-c.h"

#include "src/error_handling/error_handling.h"

/*
* Mount procedures
*/

int check_mount_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

/*
* Nfs procedures
*/

int check_getattr_proc_permissions(char *absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_setattr_proc_permissions(char *absolute_path, Nfs__SAttr *sattr, uid_t caller_uid, gid_t caller_gid);

int check_lookup_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_readlink_proc_permissions(char *symlink_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_read_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_write_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_create_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_remove_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_rename_proc_permissions(char *from_directory_absolute_path, char *to_directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_link_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_symlink_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_mkdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_rmdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_readdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid);

int check_statfs_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid);

#endif /* nfs_permissions__header__INCLUDED */