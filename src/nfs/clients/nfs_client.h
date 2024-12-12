#ifndef nfs_client__header__INCLUDED
#define nfs_client__header__INCLUDED

#include "src/serialization/nfs/nfs.pb-c.h"

/*
* Every procedure returns 0 on successful execution, and in that cases places non-NULL procedure results in the last argument that is passed to it.
* In case of unsuccessful execution, the procedure returns an error code > 0, from client_common_rpc.c.
*/

int nfs_procedure_0_do_nothing(void);

int nfs_procedure_1_get_file_attributes(Nfs__FHandle fhandle, Nfs__AttrStat *result);

int nfs_procedure_2_set_file_attributes(Nfs__SAttrArgs sattrargs, Nfs__AttrStat *result);

int nfs_procedure_4_look_up_file_name(Nfs__DirOpArgs diropargs, Nfs__DirOpRes *result);

int nfs_procedure_6_read_from_file(Nfs__ReadArgs readargs, Nfs__ReadRes *result);

int nfs_procedure_8_write_to_file(Nfs__WriteArgs writeargs, Nfs__AttrStat *result);

int nfs_procedure_9_create_file(Nfs__CreateArgs createargs, Nfs__DirOpRes *result);

int nfs_procedure_10_remove_file(Nfs__DirOpArgs diropargs, Nfs__NfsStat *result);

int nfs_procedure_14_create_directory(Nfs__CreateArgs createargs, Nfs__DirOpRes *result);

int nfs_procedure_16_read_from_directory(Nfs__ReadDirArgs readdirargs, Nfs__ReadDirRes *result);

#endif /* nfs_client__header__INCLUDED */