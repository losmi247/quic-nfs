/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: src/serialization/nfs_fh/nfs_fh.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "src/serialization/nfs_fh/nfs_fh.pb-c.h"
void   nfs_fh__nfs_file_handle__init
                     (NfsFh__NfsFileHandle         *message)
{
  static const NfsFh__NfsFileHandle init_value = NFS_FH__NFS_FILE_HANDLE__INIT;
  *message = init_value;
}
size_t nfs_fh__nfs_file_handle__get_packed_size
                     (const NfsFh__NfsFileHandle *message)
{
  assert(message->base.descriptor == &nfs_fh__nfs_file_handle__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t nfs_fh__nfs_file_handle__pack
                     (const NfsFh__NfsFileHandle *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &nfs_fh__nfs_file_handle__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t nfs_fh__nfs_file_handle__pack_to_buffer
                     (const NfsFh__NfsFileHandle *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &nfs_fh__nfs_file_handle__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
NfsFh__NfsFileHandle *
       nfs_fh__nfs_file_handle__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (NfsFh__NfsFileHandle *)
     protobuf_c_message_unpack (&nfs_fh__nfs_file_handle__descriptor,
                                allocator, len, data);
}
void   nfs_fh__nfs_file_handle__free_unpacked
                     (NfsFh__NfsFileHandle *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &nfs_fh__nfs_file_handle__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor nfs_fh__nfs_file_handle__field_descriptors[1] =
{
  {
    "inode_number",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_UINT64,
    0,   /* quantifier_offset */
    offsetof(NfsFh__NfsFileHandle, inode_number),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned nfs_fh__nfs_file_handle__field_indices_by_name[] = {
  0,   /* field[0] = inode_number */
};
static const ProtobufCIntRange nfs_fh__nfs_file_handle__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor nfs_fh__nfs_file_handle__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "nfs_fh.NfsFileHandle",
  "NfsFileHandle",
  "NfsFh__NfsFileHandle",
  "nfs_fh",
  sizeof(NfsFh__NfsFileHandle),
  1,
  nfs_fh__nfs_file_handle__field_descriptors,
  nfs_fh__nfs_file_handle__field_indices_by_name,
  1,  nfs_fh__nfs_file_handle__number_ranges,
  (ProtobufCMessageInit) nfs_fh__nfs_file_handle__init,
  NULL,NULL,NULL    /* reserved[123] */
};
