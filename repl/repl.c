#include <stdio.h>
#include <stdlib.h>

#include "rpc_programs/mount/client/mount_client.h"

/*
* The Read-Eval-Print-Loop that waits for Mount or Nfs commands and
* executed them using the mount_client.h or nfs_client.h
*/
int main() {
    int status = mount_procedure_0_do_nothing();
    if(status == 0) {
        fprintf(stdout, "Successfully executed MOUNTPROC_NULL\n");
    }
    else{
        fprintf(stdout, "MOUNTPROC_NULL failed - status %d\n", status);
    }

    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/nfs_share";
    // do not both free and free_unapcked fhstatus! Do only one!
    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);
    if(status == 0) {
        fprintf(stdout, "Successfully executed MOUNTPROC_MNT\n");
        fprintf(stdout, "Status: %d\n", fhstatus->status);
        fprintf(stdout, "Filehandle: %s\n", (unsigned char *) fhstatus->directory->handle.data);

        mount__fh_status__free_unpacked(fhstatus, NULL);
    }
    else{
        fprintf(stdout, "MOUNTPROC_MNT failed - status %d\n", status);

        free(fhstatus);
    }


    //while(1) {
        // wait for input, parse, and translate to procedures to be called
    //}
}