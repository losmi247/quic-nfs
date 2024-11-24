/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: src/serialization/nfs_fh/nfs_fh.proto */

#ifndef PROTOBUF_C_src_2fserialization_2fnfs_5ffh_2fnfs_5ffh_2eproto__INCLUDED
#define PROTOBUF_C_src_2fserialization_2fnfs_5ffh_2fnfs_5ffh_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _NfsFh__NfsFileHandle NfsFh__NfsFileHandle;


/* --- enums --- */


/* --- messages --- */

struct  _NfsFh__NfsFileHandle
{
  ProtobufCMessage base;
  /*
   * 8 bytes
   */
  uint64_t inode_number;
  /*
   * 8 bytes
   */
  int64_t timestamp;
};
#define NFS_FH__NFS_FILE_HANDLE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs_fh__nfs_file_handle__descriptor) \
    , 0, 0 }


/* NfsFh__NfsFileHandle methods */
void   nfs_fh__nfs_file_handle__init
                     (NfsFh__NfsFileHandle         *message);
size_t nfs_fh__nfs_file_handle__get_packed_size
                     (const NfsFh__NfsFileHandle   *message);
size_t nfs_fh__nfs_file_handle__pack
                     (const NfsFh__NfsFileHandle   *message,
                      uint8_t             *out);
size_t nfs_fh__nfs_file_handle__pack_to_buffer
                     (const NfsFh__NfsFileHandle   *message,
                      ProtobufCBuffer     *buffer);
NfsFh__NfsFileHandle *
       nfs_fh__nfs_file_handle__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs_fh__nfs_file_handle__free_unpacked
                     (NfsFh__NfsFileHandle *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*NfsFh__NfsFileHandle_Closure)
                 (const NfsFh__NfsFileHandle *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor nfs_fh__nfs_file_handle__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_src_2fserialization_2fnfs_5ffh_2fnfs_5ffh_2eproto__INCLUDED */
