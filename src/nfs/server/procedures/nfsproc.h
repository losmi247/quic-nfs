#ifndef nfsproc__header__INCLUDED
#define nfsproc__header__INCLUDED

#include "src/nfs/server/server.h"

#include "src/nfs/server/nfs_messages.h"
#include "src/nfs/server/file_management.h"

#include "src/nfs/server/nfs_permissions.h"

#include "src/path_building/path_building.h"

Rpc__AcceptedReply *serve_nfs_procedure_0_do_nothing(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_1_get_file_attributes(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_2_set_file_attributes(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_4_look_up_file_name(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_5_read_from_symbolic_link(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_6_read_from_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_8_write_to_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_9_create_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_10_remove_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_11_rename_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_12_create_link_to_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_13_create_symbolic_link(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_14_create_directory(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_15_remove_directory(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_16_read_from_directory(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply *serve_nfs_procedure_17_get_filesystem_attributes(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters);

#endif /* nfsproc__header__INCLUDED */