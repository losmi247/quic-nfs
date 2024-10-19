#ifndef server_common_rpc__header__INCLUDED
#define server_common_rpc__header__INCLUDED

#include <protobuf-c/protobuf-c.h>
#include "serialization/rpc/rpc.pb-c.h"

/*
* Functions implemented in server_common_rpc.c file.
*/

int send_rpc_accepted_reply_message(int rpc_client_socket_fd, Rpc__AcceptedReply accepted_reply);

int send_rpc_rejected_reply_message(int rpc_client_socket_fd, Rpc__ReplyStat reply_stat, Rpc__RejectStat reject_stat, Rpc__MismatchInfo *mismatch_info, Rpc__AuthStat *auth_stat);

void handle_client(int rpc_client_socket_fd);

/*
* Functions that must be implemented in any specific RPC program written.
* E.g. the Mount server implementation will implement the below functions.
*/

/*
* Forwards the RPC call to the specific RPC program, and returns the AcceptedReply.
*
* The specific RPC program's server takes on the responsibility to free up all heap-allocated memory 
* inside the Rpc__AcceptedReply returned from this function. For example those are procedure arguments or mismatch_info.
*/
Rpc__AcceptedReply forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

/*
* Frees up all heap allocated state from the accepted_reply returned by the RPC program's server,
* in the forward_rpc_call_to_program function.
*/
void clean_up_accepted_reply(Rpc__AcceptedReply accepted_reply);

#endif /* server_common_rpc__header__INCLUDED */