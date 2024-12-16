#include "tests/test_common.h"

#include "src/nfs/nfs_common.h"

/*
* Mount RPC program tests.
*/

TestSuite(mount_test_suite);

// MOUNTPROC_NULL (0)
Test(mount_test_suite, null, .description = "MOUNTPROC_NULL") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context();
    int status = mount_procedure_0_do_nothing(rpc_connection_context);
    free_rpc_connection_context(rpc_connection_context);
    if (status != 0) {
        cr_fatal("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

// MOUNTPROC_MNT (1)
Test(mount_test_suite, mnt_ok, .description = "MOUNTPROC_MNT ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");
    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(mount_test_suite, mnt_no_such_directory, .description = "MOUNTPROC_MNT no such directory") {
    mount_directory_fail("/non_existent_directory", MOUNT__STAT__MNTERR_NOENT);
}

Test(mount_test_suite, mnt_not_a_directory, .description = "MOUNTPROC_MNT not a directory") {
    // try to mount a file /nfs_share/test_file.txt
    mount_directory_fail("/nfs_share/test_file.txt", MOUNT__STAT__MNTERR_NOTDIR);
}

Test(mount_test_suite, mnt_directory_not_exported, .description = "MOUNTPROC_MNT directory not exported") {
    mount_directory_fail("/existent_but_non_exported_directory", MOUNT__STAT__MNTERR_NOTEXP);
}