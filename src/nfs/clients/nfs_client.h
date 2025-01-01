#ifndef nfs_client__header__INCLUDED
#define nfs_client__header__INCLUDED

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
#include "src/serialization/nfs/nfs.pb-c.h"

/*
* Every procedure returns 0 on successful execution, and in that cases places non-NULL procedure results in the last argument that is passed to it.
* In case of unsuccessful execution, the procedure returns an error code > 0, from client_common_rpc.c.
*/

int nfs_procedure_0_do_nothing(RpcConnectionContext *rpc_connection_context);

int nfs_procedure_1_get_file_attributes(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle, Nfs__AttrStat *result);

int nfs_procedure_2_set_file_attributes(RpcConnectionContext *rpc_connection_context, Nfs__SAttrArgs sattrargs, Nfs__AttrStat *result);

int nfs_procedure_4_look_up_file_name(RpcConnectionContext *rpc_connection_context, Nfs__DirOpArgs diropargs, Nfs__DirOpRes *result);

int nfs_procedure_5_read_from_symbolic_link(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle, Nfs__ReadLinkRes *result);

int nfs_procedure_6_read_from_file(RpcConnectionContext *rpc_connection_context, Nfs__ReadArgs readargs, Nfs__ReadRes *result);

int nfs_procedure_8_write_to_file(RpcConnectionContext *rpc_connection_context, Nfs__WriteArgs writeargs, Nfs__AttrStat *result);

int nfs_procedure_9_create_file(RpcConnectionContext *rpc_connection_context, Nfs__CreateArgs createargs, Nfs__DirOpRes *result);

int nfs_procedure_10_remove_file(RpcConnectionContext *rpc_connection_context, Nfs__DirOpArgs diropargs, Nfs__NfsStat *result);

int nfs_procedure_11_rename_file(RpcConnectionContext *rpc_connection_context, Nfs__RenameArgs renameargs, Nfs__NfsStat *result);

int nfs_procedure_13_create_symbolic_link(RpcConnectionContext *rpc_connection_context, Nfs__SymLinkArgs symlinkargs, Nfs__NfsStat *result);

int nfs_procedure_14_create_directory(RpcConnectionContext *rpc_connection_context, Nfs__CreateArgs createargs, Nfs__DirOpRes *result);

int nfs_procedure_15_remove_directory(RpcConnectionContext *rpc_connection_context, Nfs__DirOpArgs diropargs, Nfs__NfsStat *result);

int nfs_procedure_16_read_from_directory(RpcConnectionContext *rpc_connection_context, Nfs__ReadDirArgs readdirargs, Nfs__ReadDirRes *result);

int nfs_procedure_17_get_filesystem_attributes(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle, Nfs__StatFsRes *result);

#endif /* nfs_client__header__INCLUDED */