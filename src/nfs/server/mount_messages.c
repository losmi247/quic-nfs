#include "mount_messages.h"

/*
* Creates a FhStatus with the given non-MNT_OK status and default case.
*
* The user of this function takes the responsibility to free the Empty allocated
* in this function.
*/
Mount__FhStatus create_default_case_fh_status(Mount__Stat non_mnt_ok_status) {
    Mount__FhStatus fh_status = MOUNT__FH_STATUS__INIT;
    fh_status.status = non_mnt_ok_status;
    fh_status.fhstatus_body_case = MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    fh_status.default_case = empty;

    return fh_status;
}