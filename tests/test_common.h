#ifndef test_common__header__INCLUDED
#define test_common__header__INCLUDED

#define NONEXISTENT_INODE_NUMBER 1234567891235
#define NONEXISTENT_FILE_NAME "non_existent_file.txt"     // in your test containers, never create a file with this filename
#define NFS_SHARE_ENTRIES {"..", ".", "mkdir_test", "create_test", "write_test", "a.txt", "test_file.txt", "large_file.txt"}
#define NFS_SHARE_NUMBER_OF_ENTRIES 8

#include <time.h>

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/serialization/mount/mount.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

#include "tests/validation/procedure_validation.h"

#include "src/nfs/nfs_common.h"

NfsFh__NfsFileHandle deep_copy_nfs_filehandle(NfsFh__NfsFileHandle *nfs_filehandle);

#endif /* test_common__header__INCLUDED */