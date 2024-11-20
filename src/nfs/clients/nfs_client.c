#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/common_rpc/common_rpc.h"
#include "src/common_rpc/client_common_rpc.h"

#include "../nfs_common.h"
#include "nfs_client.h"

#include "src/serialization/rpc/rpc.pb-c.h"

/*
* Performs the NFSPROC_NULL Nfs procedure.
* On successful run, returns 0 and places procedure result in 'result' (no result here).
* On unsuccessful run, returns error code > 0.
*/
int nfs_procedure_0_do_nothing(void) {
    // no parameters, so an empty Any
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    Rpc__RpcMsg *rpc_reply = invoke_rpc(NFS_RPC_SERVER_IPV4_ADDR, NFS_RPC_SERVER_PORT, NFS_RPC_PROGRAM_NUMBER, 2, 0, parameters);

    int error_code = validate_rpc_message_from_server(rpc_reply);
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if(error_code > 0) {
        return error_code;
    }
    if(procedure_results == NULL) {
        fprintf(stderr, "NFSPROC_NULL: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' checked that procedure_results is not NULL\n");
        return 1;
    }

    // check that procedure results contain the right type
    if(procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/None") != 0) {
        fprintf(stderr, "NFSPROC_NULL: Expected nfs/None but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 2;
    }

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
* Performs the NFSPROC_GETATTR Nfs procedure.
* On successful run, returns 0 and places procedure result in 'result'.
* On unsuccessful run, returns error code > 0.
*
* In case this function returns 0, the user of this function takes responsibility 
* to call 'mount__fh_status__free_unpacked(fh_status, NULL)' on the received Mount__FhStatus eventually.
*/
int nfs_procedure_1_get_file_attributes(Nfs__FHandle fhandle, Nfs__AttrStat *result) {
    // serialize the FHandle
    size_t fhandle_size = nfs__fhandle__get_packed_size(&fhandle);
    uint8_t *fhandle_buffer = malloc(fhandle_size);
    nfs__fhandle__pack(&fhandle, fhandle_buffer);

    // Any message to wrap FHandle
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/FHandle";
    parameters.value.data = fhandle_buffer;
    parameters.value.len = fhandle_size;

    // send RPC call
    Rpc__RpcMsg *rpc_reply = invoke_rpc(NFS_RPC_SERVER_IPV4_ADDR, NFS_RPC_SERVER_PORT, NFS_RPC_PROGRAM_NUMBER, 2, 1, parameters);
    free(fhandle_buffer);

    // validate RPC reply
    int error_code = validate_rpc_message_from_server(rpc_reply);
    if(error_code > 0) {
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return error_code;
    }

    log_rpc_msg_info(rpc_reply);

    // extract procedure results
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if(procedure_results == NULL) {
        fprintf(stderr, "NFSPROC_GETATTR: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 1;
    }

    // check that procedure results contain the right type
    if(procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/AttrStat") != 0) {
        fprintf(stderr, "NFSPROC_GETATTR: Expected nfs/AttrStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 2;
    }

    // now we can unpack the AttrStat from the Any message
    Nfs__AttrStat *attr_stat = nfs__attr_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if(attr_stat == NULL) {
        fprintf(stderr, "NFSPROC_GETATTR: Failed to unpack Nfs__AttrStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 3;
    }

    // place attr_stat into the result
    *result = *attr_stat;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
* Performs the NFSPROC_SETATTR Nfs procedure.
* On successful run, returns 0 and places procedure result in 'result'.
* On unsuccessful run, returns error code > 0.
*
* In case this function returns 0, the user of this function takes responsibility 
* to call 
*/
int nfs_procedure_2_set_file_attributes(Nfs__SAttrArgs sattrargs, Nfs__AttrStat *result) {
    // serialize the SAttrArgs
    size_t sattrargs_size = nfs__sattr_args__get_packed_size(&sattrargs);
    uint8_t *sattrargs_buffer = malloc(sattrargs_size);
    nfs__sattr_args__pack(&sattrargs, sattrargs_buffer);

    // Any message to wrap SAttrArgs
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "nfs/SAttrArgs";
    parameters.value.data = sattrargs_buffer;
    parameters.value.len = sattrargs_size;

    // send RPC call
    Rpc__RpcMsg *rpc_reply = invoke_rpc(NFS_RPC_SERVER_IPV4_ADDR, NFS_RPC_SERVER_PORT, NFS_RPC_PROGRAM_NUMBER, 2, 2, parameters);
    free(sattrargs_buffer);

    // validate RPC reply
    int error_code = validate_rpc_message_from_server(rpc_reply);
    if(error_code > 0) {
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return error_code;
    }

    log_rpc_msg_info(rpc_reply);

    // extract procedure results
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if(procedure_results == NULL) {
        fprintf(stderr, "NFSPROC_SETATTR: procedure_results is NULL - This shouldn't happen, 'validated_rpc_reply' checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 1;
    }

    // check that procedure results contain the right type
    if(procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "nfs/AttrStat") != 0) {
        fprintf(stderr, "NFSPROC_SETATTR: Expected nfs/AttrStat but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 2;
    }

    // now we can unpack the AttrStat from the Any message
    Nfs__AttrStat *attr_stat = nfs__attr_stat__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if(attr_stat == NULL) {
        fprintf(stderr, "NFSPROC_SETATTR: Failed to unpack Nfs__AttrStat\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 3;
    }

    // place attr_stat into the result
    *result = *attr_stat;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}