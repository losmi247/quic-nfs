#ifndef test_common__header__INCLUDED
#define test_common__header__INCLUDED

#define NONEXISTENT_INODE_NUMBER 1234567891235

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/serialization/mount/mount.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

#include "tests/validation/procedure_validation.h"

NfsFh__NfsFileHandle deep_copy_nfs_filehandle(NfsFh__NfsFileHandle *nfs_filehandle);

#endif /* test_common__header__INCLUDED */