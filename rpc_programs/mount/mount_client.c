#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_rpc/common_rpc.h"
#include "common_rpc/client_common_rpc.h"

#include "mount_client.h"
#include "mount.h"

#include "serialization/rpc/rpc.pb-c.h"

/*
* Performs the MOUNTPROC_NULL Mount procedure.
* On successful run, returns 0 and places procedure result in 'result' (no result here).
* On unsuccessful run, returns error code > 0.
*/
int mount_procedure_0_do_nothing(void) {
    // no parameters, so an empty Any
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    Rpc__RpcMsg *rpc_reply = invoke_rpc(MOUNT_RPC_SERVER_IPV4_ADDR, MOUNT_RPC_SERVER_PORT, MOUNT_RPC_PROGRAM_NUMBER, 2, 0, parameters);

    int error_code = validate_rpc_message_from_server(rpc_reply);
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if(error_code > 0) {
        return error_code;
    }
    if(procedure_results == NULL) {
        fprintf(stderr, "This shouldn't happen, 'validated_rpc_reply' checked that procedure_results is not NULL\n");
        return 1;
    }

    // check that procedure results contain the right type
    if(procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "mount/None") != 0) {
        fprintf(stderr, "MOUNTPROC_NULL: Expected mount/None but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 2;
    }

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
* Performs the MOUNTPROC_MNT Mount procedure.
* On successful run, returns 0 and places procedure result in 'result'.
* On unsuccessful run, returns error code > 0.
*
* The user of this function takes responsibility to call 'mount__fh_status__free_unpacked(fh_status, NULL)'
* on the received Mount__FhStatus eventually.
*/
int mount_procedure_1_add_mount_entry(Mount__DirPath dirpath, Mount__FhStatus *result) {
    // serialize the DirPath
    size_t dirpath_size = mount__dir_path__get_packed_size(&dirpath);
    uint8_t *dirpath_buffer = malloc(dirpath_size);
    mount__dir_path__pack(&dirpath, dirpath_buffer);

    // Any message to wrap DirPath
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    parameters.type_url = "mount/DirPath";
    parameters.value.data = dirpath_buffer;
    parameters.value.len = dirpath_size;

    // send RPC call
    Rpc__RpcMsg *rpc_reply = invoke_rpc(MOUNT_RPC_SERVER_IPV4_ADDR, MOUNT_RPC_SERVER_PORT, MOUNT_RPC_PROGRAM_NUMBER, 2, 1, parameters);
    free(dirpath_buffer);

    // validate RPC reply
    int error_code = validate_rpc_message_from_server(rpc_reply);
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;
    Google__Protobuf__Any *procedure_results = accepted_reply->results;
    if(error_code > 0) {
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return error_code;
    }
    if(procedure_results == NULL) {
        fprintf(stderr, "MOUNTPROC_MNT: This shouldn't happen, 'validated_rpc_reply' checked that procedure_results is not NULL\n");
        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 1;
    }

    // check that procedure results contain the right type
    if(procedure_results->type_url == NULL || strcmp(procedure_results->type_url, "mount/FhStatus") != 0) {
        fprintf(stderr, "MOUNTPROC_MNT: Expected mount/FhStatus but received %s\n", procedure_results->type_url);

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 2;
    }

    // now we can unpack the FhStatus from the Any message
    Mount__FhStatus *fh_status = mount__fh_status__unpack(NULL, procedure_results->value.len, procedure_results->value.data);
    if(fh_status == NULL) {
        fprintf(stderr, "MOUNTPROC_MNT: Failed to unpack Mount__FhStatus\n");

        rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
        return 3;
    }

    // return the result in 'result'
    *result = *fh_status;

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);

    return 0;
}

/*
* Function to deep copy a protobuf FhStatus message.
*
* Returns a new instance of Mount__FhStatus (at different memory), identical to original
* at all field values. Deep copying means recursive, so original->directory which is a pointer
* (Mount_FHandle *) is also deep copied to a new Mount__FHandle instance, so that when 'original'
* is freed, the deep copy continues living independently.
*
* The user of this deep copy takes the responsibility to call 'mount__fh_status__free_unpacked' on it eventually.
*/
// DEEP COPY TRICK - 1 SERIALIZATION + 1 DESERIALIZATION
/*
/*
Mount__FhStatus* deep_copy_fh_status(const Mount__FhStatus *original) {
    size_t packed_size = mount__fh_status__get_packed_size(original);
    uint8_t *buffer = malloc(packed_size);
    mount__fh_status__pack(original, buffer);

    Mount__FhStatus *deep_copy = mount__fh_status__unpack(NULL, packed_size, buffer);

    free(buffer);

    return deep_copy;
}
*/
