#ifndef test_common__header__INCLUDED
#define test_common__header__INCLUDED

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

#include "src/nfs/nfs_common.h"

Mount__FhStatus *mount_directory(char *directory_absolute_path);

Nfs__DirOpRes *lookup_file_or_directory(Nfs__FHandle *directory_fhandle, char *filename, Nfs__FType expected_ftype);

NfsFh__NfsFileHandle deep_copy_nfs_filehandle(NfsFh__NfsFileHandle *nfs_filehandle);

void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype);

uint64_t get_time(Nfs__TimeVal *timeval);

void check_equal_fattr(Nfs__FAttr *fattr1, Nfs__FAttr *fattr2);

#endif /* test_common__header__INCLUDED */