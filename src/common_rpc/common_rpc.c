#include <stdio.h>
#include <string.h>

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

    return rpc_msg;
}

char *rpc_message_type_to_string(Rpc__MsgType msg_type) {
    switch(msg_type) {
        case RPC__MSG_TYPE__CALL:
            return strdup("RPC Call");
        case RPC__MSG_TYPE__REPLY:
            return strdup("RPC Reply");
        default:
            return strdup("Unknown");
    }
}

/*
* Prints basic information about a RPC message.
*/
void log_rpc_msg_info(Rpc__RpcMsg *rpc_msg) {
    if(rpc_msg == NULL) {
        return;
    }

    char *msg_type = rpc_message_type_to_string(rpc_msg->mtype);
    fprintf(stdout, "Received RPC message:\nxid: %u\nmessage type: %s\nbody case: %d\n", 
        rpc_msg->xid, msg_type, rpc_msg->body_case);
    fflush(stdout);
}

/*
* Prints basic information about an RPC call.
*/
void log_rpc_call_body_info(Rpc__CallBody *call_body) {
    if(call_body == NULL) {
        return;
    }

    printf("program number: %d\nprogram version: %d\nprocedure number: %d\n\n",
        call_body->prog, call_body->vers, call_body->proc);
    fflush(stdout);
}