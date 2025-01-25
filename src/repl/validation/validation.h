#ifndef validation__header__INCLUDED
#define validation__header__INCLUDED

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

int validate_mount_fh_status(Mount__FhStatus *fh_status);

int validate_nfs_attr_stat(Nfs__AttrStat *attrstat);

int validate_nfs_read_dir_res(Nfs__ReadDirRes *readdirres);

int validate_nfs_dir_op_res(Nfs__DirOpRes *diropres);

int validate_nfs_read_res(Nfs__ReadRes *readres);

int validate_nfs_read_link_res(Nfs__ReadLinkRes *readlinkres);

#endif /* validation__header__INCLUDED */