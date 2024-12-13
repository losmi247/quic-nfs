#ifndef nfsproc__header__INCLUDED
#define nfsproc__header__INCLUDED

#include "src/nfs/server/server.h"

#include "src/nfs/server/nfs_messages.h"

Rpc__AcceptedReply serve_nfs_procedure_0_do_nothing(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_1_get_file_attributes(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_2_set_file_attributes(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_4_look_up_file_name(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_6_read_from_file(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_8_write_to_file(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_9_create_file(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_10_remove_file(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_14_create_directory(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_15_remove_directory(Google__Protobuf__Any *parameters);

Rpc__AcceptedReply serve_nfs_procedure_16_read_from_directory(Google__Protobuf__Any *parameters);

#endif /* nfsproc__header__INCLUDED */