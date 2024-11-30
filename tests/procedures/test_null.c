#include "tests/test_common.h"

/*
* NFSPROC_NULL (0) tests
*/

TestSuite(nfs_null_test_suite);

Test(nfs_null_test_suite, null, .description = "NFSPROC_NULL") {
    int status = nfs_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}