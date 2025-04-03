#ifndef handlers__HEADER__INCLUDED
#define handlers__HEADER__INCLUDED

#include "src/fuse/nfs_fuse.h"

#include <errno.h>
#include <libgen.h> // dirname(), basename()
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "src/nfs/clients/nfs_client.h"

#include "src/message_validation/message_validation.h"

#include "src/fuse/path_resolution.h"

#define discard_const(ptr) ((void *)((intptr_t)(ptr)))

typedef struct CallbackData {
    int is_finished;
    int error_code;
    void *return_data;

    pthread_mutex_t lock;
    pthread_cond_t cond;
} CallbackData;

void wait_for_nfs_reply(CallbackData *callback_data);

int map_nfs_error(int nfs_status);

int nfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

int nfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);

int nfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);

int nfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);

int nfs_mknod(const char *path, mode_t mode, dev_t rdev);

int nfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi);

int nfs_truncate(const char *path, off_t size, struct fuse_file_info *fi);

int nfs_open(const char *path, struct fuse_file_info *fi);

int nfs_mkdir(const char *path, mode_t mode);

int nfs_rmdir(const char *path);

int nfs_unlink(const char *path);

int nfs_symlink(const char *target_path, const char *link_path);

int nfs_readlink(const char *path, char *buffer, size_t size);

int nfs_rename(const char *from, const char *to, unsigned int flags);

int nfs_link(const char *target_path, const char *link_path);

#endif /* handlers__HEADER__INCLUDED */