#include "nfs_client.h"

/*
 * Calls the NFSPROC_NULL Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 */
int nfs_procedure_0_do_nothing(RpcConnectionContext *rpc_connection_context) {
    // no parameters, so an empty Any
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 0, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 0, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 0, parameters);
    }

    int error_code = validate_successful_accepted_reply(rpc_reply);
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if (error_code > 0) {
        return error_code;
    }
    if (procedure_results == NULL) {
        fprintf(stderr, "NFSPROC_NULL: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/None") != 0) {
        fprintf(stderr, "NFSPROC_NULL: Expected nfs/None but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_GETATTR Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__attr_stat__free_unpacked(attrstat, NULL) on the received Nfs__AttrStat eventually.
 */
int nfs_procedure_1_get_file_attributes(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle,
                                        Nfs__AttrStat *result) {
    // serialize the FHandle
    size_t fhandle_size = nfs__fhandle__get_packed_size(&fhandle);
    uint8_t *fhandle_buffer = malloc(fhandle_size);
    nfs__fhandle__pack(&fhandle, fhandle_buffer);

    // Any message to wrap FHandle
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/FHandle";
    parameters.value.data = fhandle_buffer;
    parameters.value.len = fhandle_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 1, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 1, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 1, parameters);
    }
    free(fhandle_buffer);

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
        fprintf(stderr, "NFSPROC_GETATTR: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/AttrStat") != 0) {
        fprintf(stderr, "NFSPROC_GETATTR: Expected nfs/AttrStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the AttrStat from the Any message
    Nfs__AttrStat *attr_stat =
        nfs__attr_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (attr_stat == NULL) {
        fprintf(stderr, "NFSPROC_GETATTR: Failed to unpack Nfs__AttrStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place attr_stat into the result
    *result = *attr_stat;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_SETATTR Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__attr_stat__free_unpacked(attrstat, NULL) on the received Nfs__AttrStat eventually.
 */
int nfs_procedure_2_set_file_attributes(RpcConnectionContext *rpc_connection_context, Nfs__SAttrArgs sattrargs,
                                        Nfs__AttrStat *result) {
    // serialize the SAttrArgs
    size_t sattrargs_size = nfs__sattr_args__get_packed_size(&sattrargs);
    uint8_t *sattrargs_buffer = malloc(sattrargs_size);
    nfs__sattr_args__pack(&sattrargs, sattrargs_buffer);

    // Any message to wrap SAttrArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/SAttrArgs";
    parameters.value.data = sattrargs_buffer;
    parameters.value.len = sattrargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 2, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 2, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 2, parameters);
    }
    free(sattrargs_buffer);

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
        fprintf(stderr, "NFSPROC_SETATTR: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/AttrStat") != 0) {
        fprintf(stderr, "NFSPROC_SETATTR: Expected nfs/AttrStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the AttrStat from the Any message
    Nfs__AttrStat *attr_stat =
        nfs__attr_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (attr_stat == NULL) {
        fprintf(stderr, "NFSPROC_SETATTR: Failed to unpack Nfs__AttrStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place attr_stat into the result
    *result = *attr_stat;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_LOOKUP Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call Nfs__dir_op_res__free_unpacked(diropres) on the received Nfs__DirOpRes eventually.
 */
int nfs_procedure_4_look_up_file_name(RpcConnectionContext *rpc_connection_context, Nfs__DirOpArgs diropargs,
                                      Nfs__DirOpRes *result) {
    // serialize the DirOpArgs
    size_t diropargs_size = nfs__dir_op_args__get_packed_size(&diropargs);
    uint8_t *diropargs_buffer = malloc(diropargs_size);
    nfs__dir_op_args__pack(&diropargs, diropargs_buffer);

    // Any message to wrap DirOpArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/DirOpArgs";
    parameters.value.data = diropargs_buffer;
    parameters.value.len = diropargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 4, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 4, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 4, parameters);
    }
    free(diropargs_buffer);

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
        fprintf(stderr, "NFSPROC_LOOKUP: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/DirOpRes") != 0) {
        fprintf(stderr, "NFSPROC_LOOKUP: Expected nfs/DirOpRes but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the DirOpRes from the Any message
    Nfs__DirOpRes *diropres =
        nfs__dir_op_res__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (diropres == NULL) {
        fprintf(stderr, "NFSPROC_LOOKUP: Failed to unpack Nfs__DirOpRes\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place diropres into the result
    *result = *diropres;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_READLINK Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call Nfs__read_link_res__free_unpacked(readlinkres) on the received Nfs__ReadLinkRes eventually.
 */
int nfs_procedure_5_read_from_symbolic_link(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle,
                                            Nfs__ReadLinkRes *result) {
    // serialize the FHandle
    size_t fhandle_size = nfs__fhandle__get_packed_size(&fhandle);
    uint8_t *fhandle_buffer = malloc(fhandle_size);
    nfs__fhandle__pack(&fhandle, fhandle_buffer);

    // Any message to wrap FHandle
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/FHandle";
    parameters.value.data = fhandle_buffer;
    parameters.value.len = fhandle_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 5, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 5, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 5, parameters);
    }
    free(fhandle_buffer);

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
        fprintf(stderr, "NFSPROC_READLINK: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/ReadLinkRes") != 0) {
        fprintf(stderr, "NFSPROC_READLINK: Expected nfs/ReadLinkRes but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the ReadLinkres from the Any message
    Nfs__ReadLinkRes *readlinkres =
        nfs__read_link_res__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (readlinkres == NULL) {
        fprintf(stderr, "NFSPROC_READLINK: Failed to unpack Nfs__ReadLinkRes\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place readlinkres into the result
    *result = *readlinkres;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_READ Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call Nfs__dir_op_res__free_unpacked(diropres) on the received Nfs__DirOpRes eventually.
 */
int nfs_procedure_6_read_from_file(RpcConnectionContext *rpc_connection_context, Nfs__ReadArgs readargs,
                                   Nfs__ReadRes *result) {
    // serialize the ReadArgs
    size_t readargs_size = nfs__read_args__get_packed_size(&readargs);
    uint8_t *readargs_buffer = malloc(readargs_size);
    nfs__read_args__pack(&readargs, readargs_buffer);

    // Any message to wrap ReadArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/ReadArgs";
    parameters.value.data = readargs_buffer;
    parameters.value.len = readargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 6, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 6, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 6, parameters);
    }
    free(readargs_buffer);

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
        fprintf(stderr, "NFSPROC_READ: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/ReadRes") != 0) {
        fprintf(stderr, "NFSPROC_READ: Expected nfs/ReadRes but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the ReadRes from the Any message
    Nfs__ReadRes *readres = nfs__read_res__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (readres == NULL) {
        fprintf(stderr, "NFSPROC_READ: Failed to unpack Nfs__ReadRes\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place readres into the result
    *result = *readres;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_WRITE Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__attr_stat__free_unpacked(attrstat, NULL) on the received Nfs__AttrStat eventually.
 */
int nfs_procedure_8_write_to_file(RpcConnectionContext *rpc_connection_context, Nfs__WriteArgs writeargs,
                                  Nfs__AttrStat *result) {
    // serialize the ReadArgs
    size_t writeargs_size = nfs__write_args__get_packed_size(&writeargs);
    uint8_t *writeargs_buffer = malloc(writeargs_size);
    nfs__write_args__pack(&writeargs, writeargs_buffer);

    // Any message to wrap WriteArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/WriteArgs";
    parameters.value.data = writeargs_buffer;
    parameters.value.len = writeargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 8, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 8, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 8, parameters);
    }
    free(writeargs_buffer);

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
        fprintf(stderr, "NFSPROC_WRITE: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/AttrStat") != 0) {
        fprintf(stderr, "NFSPROC_WRITE: Expected nfs/AttrStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the AttrStat from the Any message
    Nfs__AttrStat *attrstat = nfs__attr_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (attrstat == NULL) {
        fprintf(stderr, "NFSPROC_READ: Failed to unpack Nfs__AttrStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place attrstat into the result
    *result = *attrstat;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_CREATE Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__dir_op_res__free_unpacked(diropres, NULL) on the received Nfs__DirOpRes eventually.
 */
int nfs_procedure_9_create_file(RpcConnectionContext *rpc_connection_context, Nfs__CreateArgs createargs,
                                Nfs__DirOpRes *result) {
    // serialize the CreateArgs
    size_t createargs_size = nfs__create_args__get_packed_size(&createargs);
    uint8_t *createargs_buffer = malloc(createargs_size);
    nfs__create_args__pack(&createargs, createargs_buffer);

    // Any message to wrap CreateArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/CreateArgs";
    parameters.value.data = createargs_buffer;
    parameters.value.len = createargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 9, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 9, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 9, parameters);
    }
    free(createargs_buffer);

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
        fprintf(stderr, "NFSPROC_CREATE: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/DirOpRes") != 0) {
        fprintf(stderr, "NFSPROC_CREATE: Expected nfs/DirOpRes but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the DirOpRes from the Any message
    Nfs__DirOpRes *diropres =
        nfs__dir_op_res__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (diropres == NULL) {
        fprintf(stderr, "NFSPROC_CREATE: Failed to unpack Nfs__DirOpRes\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place diropres into the result
    *result = *diropres;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_REMOVE Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__nfs_stat__free_unpacked(nfsstat, NULL) on the received Nfs__NfsStat eventually.
 */
int nfs_procedure_10_remove_file(RpcConnectionContext *rpc_connection_context, Nfs__DirOpArgs diropargs,
                                 Nfs__NfsStat *result) {
    // serialize the DirOpArgs
    size_t diropargs_size = nfs__dir_op_args__get_packed_size(&diropargs);
    uint8_t *diropargs_buffer = malloc(diropargs_size);
    nfs__dir_op_args__pack(&diropargs, diropargs_buffer);

    // Any message to wrap DirOpArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/DirOpArgs";
    parameters.value.data = diropargs_buffer;
    parameters.value.len = diropargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 10, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 10, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 10, parameters);
    }
    free(diropargs_buffer);

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
        fprintf(stderr, "NFSPROC_REMOVE: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/NfsStat") != 0) {
        fprintf(stderr, "NFSPROC_REMOVE: Expected nfs/NfsStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the NfsStat from the Any message
    Nfs__NfsStat *nfs_status = nfs__nfs_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (nfs_status == NULL) {
        fprintf(stderr, "NFSPROC_REMOVE: Failed to unpack Nfs__NfsStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place the NfsStat into the result
    *result = *nfs_status;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_RENAME Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__nfs_stat__free_unpacked(nfsstat, NULL) on the received Nfs__NfsStat eventually.
 */
int nfs_procedure_11_rename_file(RpcConnectionContext *rpc_connection_context, Nfs__RenameArgs renameargs,
                                 Nfs__NfsStat *result) {
    // serialize the RenameArgs
    size_t renameargs_size = nfs__rename_args__get_packed_size(&renameargs);
    uint8_t *renameargs_buffer = malloc(renameargs_size);
    nfs__rename_args__pack(&renameargs, renameargs_buffer);

    // Any message to wrap RenameArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/RenameArgs";
    parameters.value.data = renameargs_buffer;
    parameters.value.len = renameargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 11, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 11, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 11, parameters);
    }
    free(renameargs_buffer);

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
        fprintf(stderr, "NFSPROC_RENAME: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/NfsStat") != 0) {
        fprintf(stderr, "NFSPROC_RENAME: Expected nfs/NfsStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the NfsStat from the Any message
    Nfs__NfsStat *nfs_status = nfs__nfs_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (nfs_status == NULL) {
        fprintf(stderr, "NFSPROC_RENAME: Failed to unpack Nfs__NfsStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place the NfsStat into the result
    *result = *nfs_status;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_LINK Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__nfs_stat__free_unpacked(diropres, NULL) on the received Nfs__NfsStat eventually.
 */
int nfs_procedure_12_create_link_to_file(RpcConnectionContext *rpc_connection_context, Nfs__LinkArgs linkargs,
                                         Nfs__NfsStat *result) {
    // serialize the LinkArgs
    size_t linkargs_size = nfs__link_args__get_packed_size(&linkargs);
    uint8_t *linkargs_buffer = malloc(linkargs_size);
    nfs__link_args__pack(&linkargs, linkargs_buffer);

    // Any message to wrap LinkArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/LinkArgs";
    parameters.value.data = linkargs_buffer;
    parameters.value.len = linkargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 12, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 12, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 12, parameters);
    }
    free(linkargs_buffer);

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
        fprintf(stderr, "NFSPROC_LINK: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/NfsStat") != 0) {
        fprintf(stderr, "NFSPROC_LINK: Expected nfs/NfsStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the NfsStat from the Any message
    Nfs__NfsStat *nfsstat = nfs__nfs_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (nfsstat == NULL) {
        fprintf(stderr, "NFSPROC_LINK: Failed to unpack Nfs__NfsStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place nfsstat into the result
    *result = *nfsstat;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_SYMLINK Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__nfs_stat__free_unpacked(diropres, NULL) on the received Nfs__NfsStat eventually.
 */
int nfs_procedure_13_create_symbolic_link(RpcConnectionContext *rpc_connection_context, Nfs__SymLinkArgs symlinkargs,
                                          Nfs__NfsStat *result) {
    // serialize the SymLinkArgs
    size_t symlinkargs_size = nfs__sym_link_args__get_packed_size(&symlinkargs);
    uint8_t *symlinkargs_buffer = malloc(symlinkargs_size);
    nfs__sym_link_args__pack(&symlinkargs, symlinkargs_buffer);

    // Any message to wrap SymLinkArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/SymLinkArgs";
    parameters.value.data = symlinkargs_buffer;
    parameters.value.len = symlinkargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 13, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 13, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 13, parameters);
    }
    free(symlinkargs_buffer);

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
        fprintf(stderr, "NFSPROC_SYMLINK: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/NfsStat") != 0) {
        fprintf(stderr, "NFSPROC_SYMLINK: Expected nfs/NfsStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the NfsStat from the Any message
    Nfs__NfsStat *nfsstat = nfs__nfs_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (nfsstat == NULL) {
        fprintf(stderr, "NFSPROC_SYMLINK: Failed to unpack Nfs__NfsStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place nfsstat into the result
    *result = *nfsstat;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_MKDIR Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__dir_op_res__free_unpacked(diropres, NULL) on the received Nfs__DirOpRes eventually.
 */
int nfs_procedure_14_create_directory(RpcConnectionContext *rpc_connection_context, Nfs__CreateArgs createargs,
                                      Nfs__DirOpRes *result) {
    // serialize the CreateArgs
    size_t createargs_size = nfs__create_args__get_packed_size(&createargs);
    uint8_t *createargs_buffer = malloc(createargs_size);
    nfs__create_args__pack(&createargs, createargs_buffer);

    // Any message to wrap CreateArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/CreateArgs";
    parameters.value.data = createargs_buffer;
    parameters.value.len = createargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 14, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 14, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 14, parameters);
    }
    free(createargs_buffer);

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
        fprintf(stderr, "NFSPROC_MKDIR: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/DirOpRes") != 0) {
        fprintf(stderr, "NFSPROC_MKDIR: Expected nfs/DirOpRes but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the DirOpRes from the Any message
    Nfs__DirOpRes *diropres =
        nfs__dir_op_res__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (diropres == NULL) {
        fprintf(stderr, "NFSPROC_MKDIR: Failed to unpack Nfs__DirOpRes\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place diropres into the result
    *result = *diropres;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_RMDIR Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__nfs_stat__free_unpacked(nfsstat, NULL) on the received Nfs__NfsStat eventually.
 */
int nfs_procedure_15_remove_directory(RpcConnectionContext *rpc_connection_context, Nfs__DirOpArgs diropargs,
                                      Nfs__NfsStat *result) {
    // serialize the DirOpArgs
    size_t diropargs_size = nfs__dir_op_args__get_packed_size(&diropargs);
    uint8_t *diropargs_buffer = malloc(diropargs_size);
    nfs__dir_op_args__pack(&diropargs, diropargs_buffer);

    // Any message to wrap DirOpArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/DirOpArgs";
    parameters.value.data = diropargs_buffer;
    parameters.value.len = diropargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 15, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 15, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 15, parameters);
    }
    free(diropargs_buffer);

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
        fprintf(stderr, "NFSPROC_RMDIR: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/NfsStat") != 0) {
        fprintf(stderr, "NFSPROC_RMDIR: Expected nfs/NfsStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the NfsStat from the Any message
    Nfs__NfsStat *nfs_status = nfs__nfs_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (nfs_status == NULL) {
        fprintf(stderr, "NFSPROC_RMDIR: Failed to unpack Nfs__NfsStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place the NfsStat into the result
    *result = *nfs_status;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_READDIR Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call Nfs__read_dir_res__free_unpacked(readdirres) on the received Nfs__ReadDirRes eventually.
 */
int nfs_procedure_16_read_from_directory(RpcConnectionContext *rpc_connection_context, Nfs__ReadDirArgs readdirargs,
                                         Nfs__ReadDirRes *result) {
    // serialize the ReadDirArgs
    size_t readdirargs_size = nfs__read_dir_args__get_packed_size(&readdirargs);
    uint8_t *readdirargs_buffer = malloc(readdirargs_size);
    nfs__read_dir_args__pack(&readdirargs, readdirargs_buffer);

    // Any message to wrap ReadDirArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/ReadDirArgs";
    parameters.value.data = readdirargs_buffer;
    parameters.value.len = readdirargs_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 16, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 16, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 16, parameters);
    }
    free(readdirargs_buffer);

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
        fprintf(stderr, "NFSPROC_READDIR: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/ReadDirRes") != 0) {
        fprintf(stderr, "NFSPROC_READDIR: Expected nfs/ReadDirRes but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the ReadDirRes from the Any message
    Nfs__ReadDirRes *readdirres =
        nfs__read_dir_res__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (readdirres == NULL) {
        fprintf(stderr, "NFSPROC_READDIR: Failed to unpack Nfs__ReadDirRes\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place readdirres into the result
    *result = *readdirres;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
 * Calls the NFSPROC_STATFS Nfs procedure.
 * On successful run, returns 0 and places procedure result in 'result'.
 * On unsuccessful run, returns error code > 0 if validation of the RPC message failed - this is
 * the validation error code, and returns error code < 0 if validation of procedure results (type checking
 * and deserialization) failed.
 *
 * In case this function returns 0, the user of this function takes responsibility
 * to call nfs__stat_fs_res__free_unpacked(attrstat, NULL) on the received Nfs__StatFsRes eventually.
 */
int nfs_procedure_17_get_filesystem_attributes(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle,
                                               Nfs__StatFsRes *result) {
    // serialize the FHandle
    size_t fhandle_size = nfs__fhandle__get_packed_size(&fhandle);
    uint8_t *fhandle_buffer = malloc(fhandle_size);
    nfs__fhandle__pack(&fhandle, fhandle_buffer);

    // Any message to wrap FHandle
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/FHandle";
    parameters.value.data = fhandle_buffer;
    parameters.value.len = fhandle_size;

    // send RPC call over the desired transport protocol
    Rpc__RpcMsg *rpc_reply;
    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 17, parameters);
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        rpc_reply = invoke_rpc_remote_quic(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 17, parameters, true);
        break;
    default:
        rpc_reply = invoke_rpc_remote_tcp(rpc_connection_context, NFS_RPC_PROGRAM_NUMBER, 2, 17, parameters);
    }
    free(fhandle_buffer);

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
        fprintf(stderr, "NFSPROC_STATFS: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' "
                        "checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -1;
    }

    // check that procedure results contain the right type
    if (procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/StatFsRes") != 0) {
        fprintf(stderr, "NFSPROC_STATFS: Expected nfs/StatFsRes but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -2;
    }

    // now we can unpack the StatFsRes from the Any message
    Nfs__StatFsRes *statfsres =
        nfs__stat_fs_res__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if (statfsres == NULL) {
        fprintf(stderr, "NFSPROC_STATFS: Failed to unpack Nfs__StatFsRes\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return -3;
    }

    // place StatFsRes into the result
    *result = *statfsres;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}