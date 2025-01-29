#ifndef handlers__HEADER__INCLUDED
#define handlers__HEADER__INCLUDED

#include "src/repl/repl.h"

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "src/repl/soft_links/soft_links.h"

#include "src/nfs/clients/nfs_client.h"

#include "src/repl/validation/validation.h"

typedef struct CallbackData {
	int is_finished;
    int error_code;
	void *return_data;

    pthread_mutex_t lock;
    pthread_cond_t cond;
} CallbackData;

void wait_for_nfs_reply(CallbackData *callback_data);

int nfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

int nfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);

int nfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);

extern pthread_mutex_t nfs_mutex;

#endif /* handlers__HEADER__INCLUDED */