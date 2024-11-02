#include <stdio.h>

#include "common_rpc.h"

/*
* Generates the xid for the next RPC message (no matter whether call or reply).
*/
uint32_t generate_rpc_xid(void) {
    static int xid = 1;
    return xid++;
}

/*
* Deserializes the RPC message from the network representation back to a C structure.
* Returns NULL if deserialization was unsuccessful.
*
* The user of this function takes on the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
*/
Rpc__RpcMsg *deserialize_rpc_msg(uint8_t *rpc_msg_buffer, size_t bytes_received) {
    Rpc__RpcMsg *rpc_msg = rpc__rpc_msg__unpack(NULL, bytes_received, rpc_msg_buffer);
    if(rpc_msg == NULL) {
        fprintf(stderr, "Error unpacking received message");
        return NULL;
    }

    printf("xid: %u\n", rpc_msg->xid);
    printf("message type: %d\n", rpc_msg->mtype);
    printf("body case: %d\n", rpc_msg->body_case);

    return rpc_msg;
}