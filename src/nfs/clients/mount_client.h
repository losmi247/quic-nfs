#ifndef mount_client__header__INCLUDED
#define mount_client__header__INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/common_rpc/common_rpc.h"
#include "src/common_rpc/client_common_rpc.h"
#include "src/common_rpc/rpc_connection_context.h"

#include "src/transport/tcp/tcp_rpc_client.h"
#include "src/transport/quic/quic_rpc_client.h"

#include "../nfs_common.h"

#include "src/serialization/rpc/rpc.pb-c.h"
#include "src/serialization/mount/mount.pb-c.h"

/*
* Every procedure returns 0 on successful execution, and in that cases places non-NULL procedure results in the last argument that is passed to it.
* In case of unsuccessful execution, the procedure returns an error code > 0, from client_common_rpc.c.
*/

int mount_procedure_0_do_nothing(RpcConnectionContext *rpc_connection_context);

int mount_procedure_1_add_mount_entry(RpcConnectionContext *rpc_connection_context, Mount__DirPath dirpath, Mount__FhStatus *result);

#endif /* mount_client__header__INCLUDED */