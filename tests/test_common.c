#include "test_common.h"

/*
* Common test functions
*/

/*
* Creates a RpcConnectionContext with the test server IP address and test server port,
* and an AUTH_SYS credential+verifier pair with uid=0 (tests are a NFS client that is a root user),
* and the given transport protocol identifier.
*
* The user of this function takes the responsiblity to deallocate the received RpcConnectionContext using
* the free_rpc_connection_context() function.
*/
RpcConnectionContext *create_test_rpc_connection_context(TransportProtocol transport_protocol) {
    uint32_t gids[1] = {0};
    Rpc__OpaqueAuth *root_credential = create_auth_sys_opaque_auth("test", 0, 0, 1, gids);

    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();

    return create_rpc_connection_context(NFS_AND_MOUNT_TEST_RPC_SERVER_IPV4_ADDR, NFS_AND_MOUNT_TEST_RPC_SERVER_PORT, root_credential, verifier, transport_protocol);
}

/*
* Creates a RpcConnectionContext with the test server IP address and test server port, and the given credential and verifier.
*
* The user of this function takes the responsiblity to deallocate the received RpcConnectionContext using
* the free_rpc_connection_context() function.
*/
RpcConnectionContext *create_rpc_connection_context_with_test_ipaddr_and_port(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, TransportProtocol transport_protocol) {
    return create_rpc_connection_context(NFS_AND_MOUNT_TEST_RPC_SERVER_IPV4_ADDR, NFS_AND_MOUNT_TEST_RPC_SERVER_PORT, credential, verifier, transport_protocol);
}