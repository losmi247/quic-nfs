#ifndef nfs_fuse__HEADER__INCLUDED
#define nfs_fuse__HEADER__INCLUDED

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

#include "src/common_rpc/rpc_connection_context.h"
#include "src/serialization/nfs/nfs.pb-c.h"

/*
* Nfs client state
*/

extern RpcConnectionContext *rpc_connection_context;

extern Nfs__FHandle *filesystem_root_fhandle;

extern TransportProtocol chosen_transport_protocol;

#endif /* nfs_fuse__HEADER__INCLUDED */