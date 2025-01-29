#ifndef path_resolution__HEADER__INCLUDED
#define path_resolution__HEADER__INCLUDED

#include "src/serialization/nfs/nfs.pb-c.h"

#include "src/common_rpc/rpc_connection_context.h"

Nfs__FHandle *resolve_absolute_path(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *filesystem_root_fhandle, char *path, Nfs__FType *ftype);

#endif /* path_resolution__HEADER__INCLUDED */