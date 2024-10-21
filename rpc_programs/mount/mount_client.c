#include <stdio.h>

#include "common_rpc/common_rpc.h"
#include "common_rpc/client_common_rpc.h"

#include "mount_client.h"
#include "mount.h"

#include "serialization/rpc/rpc.pb-c.h"

void mount_procedure_0_do_nothing(void) {
    // no parameters, so an empty Any
    Google__Protobuf__Any parameters = GOOGLE__PROTOBUF__ANY__INIT;
    Rpc__RpcMsg *rpc_reply = invoke_rpc(MOUNT_RPC_SERVER_IPV4_ADDR, MOUNT_RPC_SERVER_PORT, MOUNT_RPC_PROGRAM_NUMBER, 2, 0, parameters);

    if(validate_rpc_reply(rpc_reply) > 0) {
        return;
    }

    Google__Protobuf__Any *procedure_results = extract_procedure_results_from_validated_rpc_reply(rpc_reply);
    if(procedure_results == NULL) {
        return;
    }

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
}

Mount__FhStatus mount_procedure_1_add_mount_entry(Mount__DirPath dirpath) {

}