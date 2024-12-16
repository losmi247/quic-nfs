#ifndef server_common_rpc__header__INCLUDED
#define server_common_rpc__header__INCLUDED

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"

#include "record_marking.h"

#include "src/authentication/authentication.h"

/*
* Functions implemented in server_common_rpc.c file.
*/

Rpc__AcceptedReply wrap_procedure_results_in_successful_accepted_reply(size_t results_size, uint8_t *results_buffer, char *results_type);

Rpc__AcceptedReply create_garbage_args_accepted_reply(void);

Rpc__AcceptedReply create_system_error_accepted_reply(void);

int send_rpc_accepted_reply_message(int rpc_client_socket_fd, Rpc__AcceptedReply accepted_reply);

int send_rpc_rejected_reply_message(int rpc_client_socket_fd, Rpc__ReplyStat reply_stat, Rpc__RejectStat reject_stat, Rpc__MismatchInfo *mismatch_info, Rpc__AuthStat *auth_stat);

int handle_client(int rpc_client_socket_fd);

/*
* Functions that must be implemented in any specific RPC program written.
* E.g. the Nfs+Mount server implementation will implement the below functions.
*/

Rpc__AcceptedReply forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

#endif /* server_common_rpc__header__INCLUDED */