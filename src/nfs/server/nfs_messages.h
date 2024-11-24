#ifndef nfs_messages__header__INCLUDED
#define nfs_messages__header__INCLUDED

#include <stdlib.h>

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

Nfs__AttrStat *create_default_case_attr_stat(Nfs__Stat non_nfs_ok_status);

Nfs__DirOpRes *create_default_case_dir_op_res(Nfs__Stat non_nfs_ok_status);

Nfs__ReadRes *create_default_case_read_res(Nfs__Stat non_nfs_ok_status);

#endif /* nfs_messages__header__INCLUDED */