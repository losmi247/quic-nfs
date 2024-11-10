#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include <stdlib.h>
#include <stdio.h>

#include "src/rpc_programs/mount/client/mount_client.h"

TestSuite(mount_test_suite);

Test(mount_test_suite, null, .description = "MOUNTPROC_NULL") {
    int status = mount_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

Test(mount_test_suite, mnt, .description = "MOUNTPROC_MNT") {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/nfs_share";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);
    if (status == 0) {
        // it's hard to validate the nfs filehandle at client, so we don't do it
        //cr_assert_str_eq((unsigned char *) fhstatus->directory->handle.data, nfs_share_inode_number);
        mount__fh_status__free_unpacked(fhstatus, NULL);
    } else {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }
}
