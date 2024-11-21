#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include <stdlib.h>
#include <stdio.h>

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

/*
* Make testing easier by first mounting the nfs_share, return the fhstatus on success.
*
* The user of this function takes on the responsibility to call 'mount_fh_status_free_unpacked()'
* with the obtained fhstatus in case of successful execution.
*/
Mount__FhStatus *mount_nfs_share(void) {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/nfs_share";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);
    if (status == 0) {
        // it's hard to validate the nfs filehandle at client, so we don't do it
        //cr_assert_str_eq((unsigned char *) fhstatus->directory->handle.data, nfs_share_inode_number);
        cr_assert_eq(fhstatus->status, 0);
        cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);
        cr_assert_neq((unsigned char *) fhstatus->directory, NULL);

        return fhstatus;
    } else {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);

        return NULL; // do we need to do this after cr_fail?
    }
}

/*
* Mount RPC program tests.
*/

TestSuite(mount_test_suite);

Test(mount_test_suite, null, .description = "MOUNTPROC_NULL") {
    int status = mount_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

Test(mount_test_suite, mnt, .description = "MOUNTPROC_MNT") {
    Mount__FhStatus *fhstatus = mount_nfs_share();
    mount__fh_status__free_unpacked(fhstatus, NULL);
}

/*
* Nfs RPC program tests.
*/

TestSuite(nfs_test_suite);

Test(nfs_test_suite, null, .description = "NFSPROC_NULL") {
    int status = nfs_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

Test(nfs_test_suite, getattr, .description = "NFSPROC_GETATTR") {
    Mount__FhStatus *fhstatus = mount_nfs_share();

    // now get file attributes
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.handle = fhstatus->directory->handle;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(fhandle, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_GETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    cr_assert_neq(attr_stat->attributes, NULL);

    Nfs__FAttr *fattr = attr_stat->attributes;
    cr_assert_eq(fattr->type, NFS__FTYPE__NFDIR);
    cr_assert_eq(fattr->nlink, 2); // when a directory is created, number of links to it is 2
    cr_assert_neq(fattr->atime, NULL);
    cr_assert_neq(fattr->mtime, NULL);
    cr_assert_neq(fattr->ctime, NULL);

    // other fields are difficult to validate

    mount__fh_status__free_unpacked(fhstatus, NULL);

    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

Test(nfs_test_suite, setattr, .description = "NFSPROC_SETATTR") {
    Mount__FhStatus *fhstatus = mount_nfs_share();

    // now update attributes of /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.handle = fhstatus->directory->handle;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = 0;
    sattr.uid = 0;   // make it a root-owned file
    sattr.gid = 0;
    sattr.size = -1; // can't update size on a directory
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = &fhandle;
    sattrargs.attributes = &sattr;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(sattrargs, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_SETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    cr_assert_neq(attr_stat->attributes, NULL);

    Nfs__FAttr *fattr = attr_stat->attributes;
    cr_assert_eq(fattr->type, NFS__FTYPE__NFDIR);
    cr_assert_neq(fattr->atime, NULL);
    cr_assert_eq(fattr->atime->seconds, sattr.atime->seconds);
    cr_assert_eq(fattr->atime->useconds, sattr.atime->useconds);
    cr_assert_neq(fattr->mtime, NULL);
    cr_assert_eq(fattr->mtime->seconds, sattr.mtime->seconds);
    cr_assert_eq(fattr->mtime->useconds, sattr.mtime->useconds);
    cr_assert_neq(fattr->ctime, NULL);

    cr_assert_eq(((fattr->mode) & ((1<<12)-1)), sattr.mode); // only check first 12 bits - (sticky,setuid,setgid),owner,group,others, before that is device/file type
    cr_assert_eq(fattr->uid, sattr.uid);
    cr_assert_eq(fattr->gid, sattr.gid);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}
