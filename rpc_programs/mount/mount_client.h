#ifndef mount_client__header__INCLUDED
#define mount_client__header__INCLUDED

#include "serialization/mount/mount.pb-c.h"

// 0 - MNTPROC_NULL
void mount_procedure_0_do_nothing(void);

// 1 - MNTPROC_MNT
Mount__FhStatus mount_procedure_1_add_mount_entry(Mount__DirPath);

#endif /* mount_client__header__INCLUDED */