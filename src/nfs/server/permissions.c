#include "permissions.h"

/*
* Returns 0 if the given uid/gid specifies the root user and 1 otherwise.
*/
int check_root_user(uid_t uid, gid_t gid) {
    return uid == 0 ? 0 : 1;
}

/*
* Checks whether the caller specified by the given 'caller_uid', 'caller_gid' has read permission
* on the file/directory at the given 'absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has read permission and 1 if it does not have read permission.
*/
int check_read_permission(char *absolute_path, uid_t caller_uid, gid_t caller_gid) {
    struct stat file_stat;

    if(lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return -1;
    }

    // check owner permissions
    if(caller_uid == file_stat.st_uid) {
        if(file_stat.st_mode & S_IRUSR) {
            return 0;
        }
    }
    // check group permissions
    else if(caller_gid == file_stat.st_gid) {
        if(file_stat.st_mode & S_IRGRP) {
            return 0;
        }
    }
    // check others' permissions
    else {
        if(file_stat.st_mode & S_IROTH) {
            return 0;
        }
    }

    // caller does not have read permission
    return 1;
}


/*
* Checks whether the caller specified by the given 'caller_uid', 'caller_gid' has write permission
* on the file/directory at the given 'absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has write permission and 1 if it does not have write permission.
*/
int check_write_permission(char *absolute_path, uid_t caller_uid, gid_t caller_gid) {
    struct stat file_stat;

    if(lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return -1;
    }

    // check owner permissions
    if(caller_uid == file_stat.st_uid) {
        if(file_stat.st_mode & S_IWUSR) {
            return 0;
        }
    }
    // check group permissions
    else if(caller_gid == file_stat.st_gid) {
        if(file_stat.st_mode & S_IWGRP) {
            return 0;
        }
    }
    // check others' permissions
    else {
        if(file_stat.st_mode & S_IWOTH) {
            return 0;
        }
    }

    // caller does not have write permission
    return 1;
}



/*
* Checks whether the caller specified by the given 'caller_uid', 'caller_gid' has execute permission
* on the file/directory at the given 'absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has execute permission and 1 if it does not have execute permission.
*/
int check_execute_permission(char *absolute_path, uid_t caller_uid, gid_t caller_gid) {
    struct stat file_stat;

    if(lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return -1;
    }

    // check owner permissions
    if(caller_uid == file_stat.st_uid) {
        if(file_stat.st_mode & S_IXUSR) {
            return 0;
        }
    }
    // check group permissions
    else if(caller_gid == file_stat.st_gid) {
        if(file_stat.st_mode & S_IXGRP) {
            return 0;
        }
    }
    // check others' permissions
    else {
        if(file_stat.st_mode & S_IXOTH) {
            return 0;
        }
    }

    // caller does not have execute permission
    return 1;
}

/*
* Procedure permission checking
*/

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to mount the
* directory at absolute path 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_mount_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    // no special permissions are needed to mount a remote directory
    return 0;
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to get the
* attributes of the file/directory at absolute path 'absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_getattr_proc_permissions(char *absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    // no special permissions are needed to get the attributes of a file/directory
    return 0;
}

/*
* Given the SAttr, checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to 
* set the attributes of the file/directory at absolute path 'absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_setattr_proc_permissions(char *absolute_path, Nfs__SAttr *sattr, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    if(sattr == NULL) {
        return -1;
    }

    // only root user can change ownership
    if(sattr->uid != -1 || sattr->gid != -1) {
        return check_root_user(caller_uid, caller_gid);
    }

    // otherwise setting anything else requires write permission
    if(sattr->mode != -1 || sattr->size != -1 || sattr->atime->seconds != -1 || sattr->atime->useconds != -1 || sattr->mtime->seconds != -1 || sattr->mtime->useconds != -1) {
        return check_write_permission(absolute_path, caller_uid, caller_gid);
    }

    return 0;
}

/*
* Given the absolute path of the containing directory, checks if the caller given by 'caller_uid', 
* 'caller_gid' has correct permissions to lookup a file/directory inside that containing directory.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_lookup_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_read_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to read
* the file at 'file_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_read_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_read_permission(file_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to write to
* the file at 'file_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_write_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(file_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to create a file 
* inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_create_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to remove a file 
* inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_remove_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to move a file 
* inside the directory at 'from_directory_absolute_path' to directory at 'to_directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_rename_proc_permissions(char *from_directory_absolute_path, char *to_directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    int stat = check_write_permission(from_directory_absolute_path, caller_uid, caller_gid);
    if(stat < 0) {
        return -1;
    }
    if(stat == 1) {
        return 1;
    }

    return check_write_permission(to_directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to create a
* symbolic link inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_symlink_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to create a
* directory inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_mkdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to remove a directory 
* inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_rmdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to read entries in
* the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_readdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_execute_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to get attributes of
* the file system which contains the file at 'file_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_statfs_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    // no special permissions are needed to get filesystem attributes
    return 0;
}