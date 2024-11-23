#include "mount_messages.h"

/*
* Creates a FhStatus with status > 0 equal to 'status' argument and default case.
*
* status = 1 -> the directory that client tried to mount is not exported for NFS
* status = 2 -> the directory that client tried to does not exist
*
* The user of this function takes the responsibility to free the Empty allocated
* in this function.
*/
Mount__FhStatus create_default_case_fh_status(int non_zero_status) {
    Mount__FhStatus fh_status = MOUNT__FH_STATUS__INIT;
    fh_status.status = non_zero_status;
    fh_status.fhstatus_body_case = MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    fh_status.default_case = empty;

    return fh_status;
}