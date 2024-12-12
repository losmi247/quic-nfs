#ifndef mntproc__header__INCLUDED
#define mntproc__header__INCLUDED

#include "src/nfs/server/server.h"

#include "src/nfs/server/mount_messages.h"

Rpc__AcceptedReply serve_mnt_procedure_0_do_nothing(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_mnt_procedure_1_add_mount_entry(Google__Protobuf__Any *parameters);

#endif /* mntproc__header__INCLUDED */