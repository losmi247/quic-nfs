#include "tests/test_common.h"

/*
* NFSPROC_NULL (0) tests
*/

TestSuite(nfs_null_test_suite);

Test(nfs_null_test_suite, null, .description = "NFSPROC_NULL") {
    int status = nfs_procedure_0_do_nothing(NFS_AND_MOUNT_TEST_RPC_SERVER_IPV4_ADDR, NFS_AND_MOUNT_TEST_RPC_SERVER_PORT);
    if (status != 0) {
        cr_fatal("MOUNTPROC_NULL failed - status %d\n", status);
    }
}