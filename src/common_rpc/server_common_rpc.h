#ifndef server_common_rpc__header__INCLUDED
#define server_common_rpc__header__INCLUDED

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/transport/tcp/tcp_record_marking.h"

#include "src/authentication/authentication.h"

/*
 * Functions implemented in server_common_rpc.c file.
 */

/*
 * AcceptedReply
 */

Rpc__AcceptedReply *wrap_procedure_results_in_successful_accepted_reply(size_t results_size, uint8_t *results_buffer,
                                                                        char *results_type);

Rpc__AcceptedReply *create_prog_mismatch_accepted_reply(uint32_t low, uint32_t high);

Rpc__AcceptedReply *create_default_case_accepted_reply(Rpc__AcceptStat default_case_accept_stat);

Rpc__AcceptedReply *create_garbage_args_accepted_reply(void);

Rpc__AcceptedReply *create_system_error_accepted_reply(void);

void free_accepted_reply(Rpc__AcceptedReply *accepted_reply);

/*
 * RejectedReply
 */

Rpc__RejectedReply *create_rpc_mismatch_rejected_reply(uint32_t low, uint32_t high);

Rpc__RejectedReply *create_auth_error_rejected_reply(Rpc__AuthStat stat);

void free_rejected_reply(Rpc__RejectedReply *rejected_reply);

/*
 * Functions that must be implemented in any specific RPC program written.
 * E.g. the Nfs+Mount server implementation will implement the below functions.
 */

Rpc__AcceptedReply *forward_rpc_call_to_program(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier,
                                                uint32_t program_number, uint32_t program_version,
                                                uint32_t procedure_number, Google__Protobuf__Any *parameters);

#endif /* server_common_rpc__header__INCLUDED */