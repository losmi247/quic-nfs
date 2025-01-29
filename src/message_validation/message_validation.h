#ifndef message_validation__HEADER__INCLUDED
#define message_validation__HEADER__INCLUDED

#include "src/serialization/mount/mount.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

int validate_mount_fh_status(Mount__FhStatus *fh_status);

int validate_nfs_nfs_stat(Nfs__NfsStat *nfsstat);

int validate_nfs_attr_stat(Nfs__AttrStat *attrstat);

int validate_nfs_read_dir_res(Nfs__ReadDirRes *readdirres);

int validate_nfs_dir_op_res(Nfs__DirOpRes *diropres);

int validate_nfs_read_res(Nfs__ReadRes *readres);

int validate_nfs_read_link_res(Nfs__ReadLinkRes *readlinkres);

#endif /* message_validation__HEADER__INCLUDED */