#include "tests/test_common.h"

/*
 * NFSPROC_NULL (0) tests
 */

TestSuite(nfs_null_test_suite);

Test(nfs_null_test_suite, null, .description = "NFSPROC_NULL") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("NFSPROC_NULL failued - failed to connect to the server\n");
    }
    int status = nfs_procedure_0_do_nothing(rpc_connection_context);
    free_rpc_connection_context(rpc_connection_context);
    if (status != 0) {
        cr_fatal("NFSPROC_NULL failed - status %d\n", status);
    }
}