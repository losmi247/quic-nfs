#include "test_common.h"

/*
* Common test functions
*/

/*
* Creates a RpcConnectionContext with the test server IP address and test server port,
* and the credential and verifier using AUTH_NONE.
*
* The user of this function takes the responsiblity to deallocate the received RpcConnectionContext using
* the free_rpc_connection_context() function.
*/
RpcConnectionContext *create_test_rpc_connection_context(void) {
    return create_auth_none_rpc_connection_context(NFS_AND_MOUNT_TEST_RPC_SERVER_IPV4_ADDR, NFS_AND_MOUNT_TEST_RPC_SERVER_PORT);
}