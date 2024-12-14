#ifndef test_common__header__INCLUDED
#define test_common__header__INCLUDED

// Mount and Nfs server are the same process
#define NFS_AND_MOUNT_TEST_RPC_SERVER_IPV4_ADDR "192.168.100.1"
#define NFS_AND_MOUNT_TEST_RPC_SERVER_PORT 3000

#define NONEXISTENT_INODE_NUMBER 1234567891235
#define NONEXISTENT_FILENAME "non_existent_file"     // in your test containers, never create a file or directory with this filename
#define NFS_SHARE_ENTRIES {"..", ".", "mkdir_test", "create_test", "remove_test", "rename_test", "write_test", "rmdir_test", "a.txt", "test_file.txt", "large_file.txt"}
#define NFS_SHARE_NUMBER_OF_ENTRIES 11

#include <time.h>

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/serialization/mount/mount.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

#include "tests/validation/procedure_validation.h"

#include "src/nfs/nfs_common.h"

#include "src/repl/filehandle_management.h"
#include "src/parsing/parsing.h"

#endif /* test_common__header__INCLUDED */