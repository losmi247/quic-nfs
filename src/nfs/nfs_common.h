#ifndef nfs_common__header__INCLUDED
#define nfs_common__header__INCLUDED

#define NFS_VERSION_LOW 2
#define NFS_VERSION_HIGH 2

#define NFS_RPC_PROGRAM_NUMBER 10003
#define MOUNT_RPC_PROGRAM_NUMBER 10005

#define NFS_MAXDATA 8192        // max number of bytes of data in a read/write request
#define NFS_MAXPATHLEN 1024     // max number of bytes in a pathname (Path protobuf message)
#define NFS_MAXNAMLEN 255       // max number of bytes in a file name (FileName protobuf message)
#define NFS_COOKIESIZE 4        // size in bytes of the opaque cookie passed by READDIR procedure
#define NFS_FHSIZE 32           // size in bytes of the nfs filehandle

#endif /* nfs_common__header__INCLUDED */