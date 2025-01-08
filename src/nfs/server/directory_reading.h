#ifndef directory_reading__header__INCLUDED
#define directory_reading__header__INCLUDED

#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>

#include "src/serialization/rpc/rpc.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

typedef struct ReadDirSession {
    // identification of the client who owns this session
    Rpc__AuthSysParams *client_authsysparams;

    // the directory being read
    char *directory_absolute_path;            // one of the many possible absolute paths
    ino_t directory_inode_number;
    DIR *directory_stream;
    uint64_t last_readdir_rpc_timestamp;      // time of the last READDIR RPC that read from this directory stream
} ReadDirSession;

typedef struct ReadDirSessionsList {
    ReadDirSession *readdir_session;
    struct ReadDirSessionsList *next;
} ReadDirSessionsList;

void clean_up_readdir_sessions_list(ReadDirSessionsList *readdir_sessions);

void clean_up_expired_readdir_sessions(ReadDirSessionsList **readdir_sessions);

void clean_up_directory_entries_list(Nfs__DirectoryEntriesList *directory_entries_list_head);

int read_from_directory(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path, ino_t directory_inode_number, ReadDirSessionsList **readdir_sessions, long offset_cookie, size_t byte_count, Nfs__DirectoryEntriesList **head, int *end_of_stream);

#endif /* directory_reading__header__INCLUDED */