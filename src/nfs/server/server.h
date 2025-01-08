#ifndef server__header__INCLUDED
#define server__header__INCLUDED

#define _POSIX_C_SOURCE 200809L // to be able to use truncate() in unistd.h

#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/time.h>
#include <unistd.h> // read(), write(), close()
#include <utime.h> // changing atime, ctime
#include <signal.h> // for registering the SIGTERM handler

#include "src/common_rpc/server_common_rpc.h"
#include "src/transport/tcp/tcp_rpc_server.h"
#include "src/transport/quic/quic_rpc_server.h"
#include "src/transport/transport_common.h"

#include "src/serialization/rpc/rpc.pb-c.h"
#include "src/serialization/mount/mount.pb-c.h"

#include "src/nfs/nfs_common.h"

#include "mount_list.h"
#include "inode_cache.h"
#include "readdir_sessions.h"

#include "tests/test_common.h"         // for NFS_AND_MOUNT_TEST_RPC_SERVER_PORT
#include "src/parsing/parsing.h"       // for parsing the port number from command line args

/*
* Functions implemented by RPC programs (Mount and Nfs).
*/

extern Rpc__AcceptedReply *call_mount(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

extern Rpc__AcceptedReply *call_nfs(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

/*
* Mount+Nfs server state.
*/

extern TransportProtocol transport_protocol;

extern Mount__MountList *mount_list;
extern InodeCache inode_cache;

extern ReadDirSessionsList *readdir_sessions_list;

#endif /* server__header__INCLUDED */