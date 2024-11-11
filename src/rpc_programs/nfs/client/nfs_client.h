#ifndef nfs_client__header__INCLUDED
#define nfs_client__header__INCLUDED

#include "src/serialization/nfs/nfs.pb-c.h"

/*
* Every procedure returns 0 on successful execution, and in that cases places non-NULL procedure results in the last argument that is passed to it.
* In case of unsuccessful execution, the procedure returns an error code > 0, from client_common_rpc.c.
*/

int nfs_procedure_0_do_nothing(void);

int nfs_procedure_1_get_file_attributes(Nfs__FHandle fhandle, Nfs__AttrStat *result);

#endif /* nfs_client__header__INCLUDED */