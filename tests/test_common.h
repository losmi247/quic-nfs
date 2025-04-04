#ifndef test_common__header__INCLUDED
#define test_common__header__INCLUDED

#ifndef TEST_TRANSPORT_PROTOCOL
#define TEST_TRANSPORT_PROTOCOL TRANSPORT_PROTOCOL_TCP // by default we use TCP in tests
#endif

// Mount and Nfs server are the same process
#define NFS_AND_MOUNT_TEST_RPC_SERVER_IPV4_ADDR "192.168.100.1"
#define NFS_AND_MOUNT_TEST_RPC_SERVER_PORT 3000

#define DOCKER_IMAGE_TESTUSER_UID 1500
#define NON_DOCKER_IMAGE_TESTUSER_UID 1501 // needed to make server return NFSERRR_ACCES in tests
#define DOCKER_IMAGE_TESTUSER_GID 2000
#define NON_DOCKER_IMAGE_TESTUSER_GID 2001 // needed to make server return NFSERRR_ACCES in tests

#define NONEXISTENT_INODE_NUMBER 1234567891235
#define NONEXISTENT_FILENAME                                                                                           \
    "non_existent_file" // in your test containers, never create a file or directory with this filename
#define NFS_SHARE_ENTRIES                                                                                              \
    {                                                                                                                  \
        "..", ".", "write_test", "readlink_test", "create_test", "remove_test", "rename_test", "link_test",            \
            "symlink_test", "mkdir_test", "rmdir_test", "permission_test", "a.txt", "test_file.txt", "large_file.txt"  \
    }
#define NFS_SHARE_NUMBER_OF_ENTRIES 15

#include <time.h>

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/serialization/mount/mount.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

#include "tests/validation/procedure_validation.h"

#include "src/nfs/nfs_common.h"

#include "src/filehandle_management/filehandle_management.h"
#include "src/parsing/parsing.h"

#include "src/common_rpc/rpc_connection_context.h"

#include "src/transport/transport_common.h"

/*
 * Common test functions
 */

RpcConnectionContext *create_test_rpc_connection_context(TransportProtocol transport_protocol);

RpcConnectionContext *create_rpc_connection_context_with_test_ipaddr_and_port(Rpc__OpaqueAuth *credential,
                                                                              Rpc__OpaqueAuth *verifier,
                                                                              TransportProtocol transport_protocol);

#endif /* test_common__header__INCLUDED */