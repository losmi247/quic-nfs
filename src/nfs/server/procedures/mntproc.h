#ifndef mntproc__header__INCLUDED
#define mntproc__header__INCLUDED

#include "src/nfs/server/server.h"

#include "src/nfs/server/mount_messages.h"

#include "src/nfs/server/nfs_permissions.h"

Rpc__AcceptedReply *serve_mnt_procedure_0_do_nothing(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_mnt_procedure_1_add_mount_entry(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

#endif /* mntproc__header__INCLUDED */