#ifndef nfs_common__header__INCLUDED
#define nfs_common__header__INCLUDED

#define NFS_RPC_SERVER_IPV4_ADDR "192.168.100.1"
#define NFS_RPC_SERVER_PORT 3000
#define NFS_RPC_PROGRAM_NUMBER 10003

// Mount and Nfs server are the same process
#define MOUNT_RPC_SERVER_IPV4_ADDR "192.168.100.1"
#define MOUNT_RPC_SERVER_PORT 3000
#define MOUNT_RPC_PROGRAM_NUMBER 10005

#define MAXDATA 8192        // max number of bytes of data in a read/write request
#define MAXPATHLEN 1024     // max number of bytes in a pathname (Path protobuf message)
#define MAXNAMLEN 255       // max number of bytes in a file name (FileName protobuf message)
#define COOKIESIZE 4        // size in bytes of the opaque cookie passed by READDIR operation
#define FHSIZE 32           // size in bytes of the nfs filehandle

#endif /* nfs_common__header__INCLUDED */