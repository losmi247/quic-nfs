#ifndef validation__header__INCLUDED
#define validation__header__INCLUDED

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

int validate_mount_fh_status(Mount__FhStatus *fh_status);

int validate_nfs_read_dir_res(Nfs__ReadDirRes *readdirres);

int validate_nfs_dir_op_res(Nfs__DirOpRes *diropres);

#endif /* validation__header__INCLUDED */