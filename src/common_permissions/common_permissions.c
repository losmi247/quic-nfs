#include "common_permissions.h"

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

    if (lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return -1;
    }

    // check owner permissions
    if (caller_uid == file_stat.st_uid) {
        if (file_stat.st_mode & S_IRUSR) {
            return 0;
        }
    }
    // check group permissions
    else if (caller_gid == file_stat.st_gid) {
        if (file_stat.st_mode & S_IRGRP) {
            return 0;
        }
    }
    // check others' permissions
    else {
        if (file_stat.st_mode & S_IROTH) {
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

    if (lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return -1;
    }

    // check owner permissions
    if (caller_uid == file_stat.st_uid) {
        if (file_stat.st_mode & S_IWUSR) {
            return 0;
        }
    }
    // check group permissions
    else if (caller_gid == file_stat.st_gid) {
        if (file_stat.st_mode & S_IWGRP) {
            return 0;
        }
    }
    // check others' permissions
    else {
        if (file_stat.st_mode & S_IWOTH) {
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

    if (lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return -1;
    }

    // check owner permissions
    if (caller_uid == file_stat.st_uid) {
        if (file_stat.st_mode & S_IXUSR) {
            return 0;
        }
    }
    // check group permissions
    else if (caller_gid == file_stat.st_gid) {
        if (file_stat.st_mode & S_IXGRP) {
            return 0;
        }
    }
    // check others' permissions
    else {
        if (file_stat.st_mode & S_IXOTH) {
            return 0;
        }
    }

    // caller does not have execute permission
    return 1;
}