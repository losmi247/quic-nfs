#ifndef mount_client__header__INCLUDED
#define mount_client__header__INCLUDED

#include "src/serialization/mount/mount.pb-c.h"

/*
* Every procedure returns 0 on successful execution, and in that cases places non-NULL procedure results in the last argument that is passed to it.
* In case of unsuccessful execution, the procedure returns an error code > 0, from client_common_rpc.c.
*/

int mount_procedure_0_do_nothing(void);

int mount_procedure_1_add_mount_entry(Mount__DirPath dirpath, Mount__FhStatus *result);

#endif /* mount_client__header__INCLUDED */