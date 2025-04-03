#include "mount_messages.h"

/*
 * Creates a FhStatus with the given non-MNT_OK status and default case.
 *
 * The user of this function takes the responsibility to free the FhStatus, MntStat,
 * and Empty allocated in this function.
 */
Mount__FhStatus *create_default_case_fh_status(Mount__Stat non_mnt_ok_status) {
    Mount__FhStatus *fh_status = malloc(sizeof(Mount__FhStatus));
    mount__fh_status__init(fh_status);

    Mount__MntStat *mnt_status = malloc(sizeof(Mount__MntStat));
    mount__mnt_stat__init(mnt_status);
    mnt_status->stat = non_mnt_ok_status;

    fh_status->mnt_status = mnt_status;
    fh_status->fhstatus_body_case = MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    fh_status->default_case = empty;

    return fh_status;
}