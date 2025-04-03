#include "mount_client.h"

/*
 * Performs the MOUNTPROC_NULL Mount procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 */
int mount_procedure_0_do_nothing(RpcConnectionContext *rpc_connection_context) {
    // no parameters, so an empty Any
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, MOUNT_RPC_PROGRAM_NUMBER, 2, 0, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, MOUNT_RPC_PROGRAM_NUMBER, 2, 0, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, MOUNT_RPC_PROGRAM_NUMBER, 2, 0, parameters);
    }

    int error_code = validate_successful_accepted_reply(rpc_reply);
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if (error_code > 0) {
        return error_code;
    }
    if (procedure_results == NULL) {
        fprintf(stderr, "This shouldn't happen, 'validated_rpc_reply' checked that procedure_results is not NULL\n");
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "mount/None") != 0) {
        fprintf(stderr, "MOUNTPROC_NULL: Expected mount/None but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Performs the MOUNTPROC_MNT Mount procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call 'mount__fh_status__free_unpacked(fh_status, NULL)' on the received Mount__FhStatus eventually.
 */
int mount_procedure_1_add_mount_entry(RpcConnectionContext *rpc_connection_context, Mount__DirPath dirpath,
                                      Mount__FhStatus *result) {
    // serialize the DirPath
    size_t dirpath_size = mount__dir_path__get_packed_size(&dirpath);
    uint8_t *dirpath_buffer = malloc(dirpath_size);
    mount__dir_path__pack(&dirpath, dirpath_buffer);

    // Any message to wrap DirPath
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "mount/DirPath";
    parameters.value.data = dirpath_buffer;
    parameters.value.len = dirpath_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, MOUNT_RPC_PROGRAM_NUMBER, 2, 1, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, MOUNT_RPC_PROGRAM_NUMBER, 2, 1, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, MOUNT_RPC_PROGRAM_NUMBER, 2, 1, parameters);
    }
    free(dirpath_buffer);

    // validate RPC reply
    int error_code = validate_successful_accepted_reply(rpc_reply);
    if (error_code > 0) {
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return error_code;
    }

    log_rpc_msg_info(rpc_reply);

    // extract procedure results
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if (procedure_results == NULL) {
        fprintf(
            stderr,
            "MOUNTPROC_MNT: This shouldn't happen, 'validated_rpc_reply' checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "mount/FhStatus") != 0) {
        fprintf(stderr, "MOUNTPROC_MNT: Expected mount/FhStatus but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the FhStatus from the Any message
    Mount__FhStatus *fh_status =
        mount__fh_status__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (fh_status == NULL) {
        fprintf(stderr, "MOUNTPROC_MNT: Failed to unpack Mount__FhStatus\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place fh_status into the result
    *result = *fh_status;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}
