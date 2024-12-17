#include "test_common.h"

/*
* Common test functions
*/

/*
* Creates a RpcConnectionContext with the test server IP address and test server port,
* and an AUTH_SYS credential+verifier pair with uid=0 (tests are a NFS client that is a root user).
*
* The user of this function takes the responsiblity to deallocate the received RpcConnectionContext using
* the free_rpc_connection_context() function.
*/
RpcConnectionContext *create_test_rpc_connection_context(void) {
    uint32_t gids[1] = {0};
    Rpc__OpaqueAuth *root_credential = create_auth_sys_opaque_auth("test", 0, 0, 1, gids);

    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();

    return create_rpc_connection_context(NFS_AND_MOUNT_TEST_RPC_SERVER_IPV4_ADDR, NFS_AND_MOUNT_TEST_RPC_SERVER_PORT, root_credential, verifier);
}