#ifndef readdir_sessions__header__INCLUDED
#define readdir_sessions__header__INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>

#include "src/error_handling/error_handling.h"

#include "src/serialization/rpc/rpc.pb-c.h"

typedef struct ReadDirSession {
    // identification of the client who owns this session
    Rpc__AuthSysParams *client_authsysparams;

    // the directory being read
    char *directory_absolute_path;            // one of the many possible absolute paths
    ino_t directory_inode_number;
    DIR *directory_stream;
    uint64_t last_readdir_rpc_timestamp;      // time of the last READDIR RPC that read from this directory stream
} ReadDirSession;

ReadDirSession *create_new_readdir_session(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path, ino_t directory_inode_number);

void free_readdir_session(ReadDirSession *readdir_session);

typedef struct ReadDirSessionsList {
    ReadDirSession *readdir_session;
    struct ReadDirSessionsList *next;
} ReadDirSessionsList;

int add_new_readdir_session(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path, ino_t directory_inode_number, ReadDirSessionsList **readdir_sessions);

int find_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number, ReadDirSessionsList *readdir_sessions);

ReadDirSession *get_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number, ReadDirSessionsList *readdir_sessions);

int remove_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number, ReadDirSessionsList **readdir_sessions);

void clean_up_readdir_sessions_list(ReadDirSessionsList *readdir_sessions);

#endif /* readdir_sessions__header__INCLUDED */