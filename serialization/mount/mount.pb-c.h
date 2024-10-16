/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: serialization/mount/mount.proto */

#ifndef PROTOBUF_C_serialization_2fmount_2fmount_2eproto__INCLUDED
#define PROTOBUF_C_serialization_2fmount_2fmount_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Nfs__Mount__Empty Nfs__Mount__Empty;
typedef struct _Nfs__Mount__FHandle Nfs__Mount__FHandle;
typedef struct _Nfs__Mount__FhStatus Nfs__Mount__FhStatus;
typedef struct _Nfs__Mount__DirPath Nfs__Mount__DirPath;
typedef struct _Nfs__Mount__Name Nfs__Mount__Name;
typedef struct _Nfs__Mount__MountList Nfs__Mount__MountList;
typedef struct _Nfs__Mount__Groups Nfs__Mount__Groups;
typedef struct _Nfs__Mount__ExportList Nfs__Mount__ExportList;


/* --- enums --- */


/* --- messages --- */

/*
 * Define an empty message for procedures that don't require parameters
 */
struct  _Nfs__Mount__Empty
{
  ProtobufCMessage base;
};
#define NFS__MOUNT__EMPTY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__empty__descriptor) \
     }


/*
 * The "fhandle" type, a file handle represented as a byte array
 */
struct  _Nfs__Mount__FHandle
{
  ProtobufCMessage base;
  ProtobufCBinaryData handle;
};
#define NFS__MOUNT__FHANDLE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__fhandle__descriptor) \
    , {0,NULL} }


/*
 * The "fhstatus" type, a union-like structure to indicate status or file handle
 */
struct  _Nfs__Mount__FhStatus
{
  ProtobufCMessage base;
  /*
   * 0 for success, non-zero for error
   */
  uint32_t status;
  /*
   * Valid only when status is 0
   */
  Nfs__Mount__FHandle *directory;
};
#define NFS__MOUNT__FH_STATUS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__fh_status__descriptor) \
    , 0, NULL }


/*
 * The "dirpath" type, a string representing a server pathname
 */
struct  _Nfs__Mount__DirPath
{
  ProtobufCMessage base;
  char *path;
};
#define NFS__MOUNT__DIR_PATH__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__dir_path__descriptor) \
    , (char *)protobuf_c_empty_string }


/*
 * The "name" type, an arbitrary string used for hostnames and group names
 */
struct  _Nfs__Mount__Name
{
  ProtobufCMessage base;
  char *name;
};
#define NFS__MOUNT__NAME__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__name__descriptor) \
    , (char *)protobuf_c_empty_string }


/*
 * The "mountlist" type, a linked list of mounted entries
 */
struct  _Nfs__Mount__MountList
{
  ProtobufCMessage base;
  Nfs__Mount__Name *hostname;
  Nfs__Mount__DirPath *directory;
  /*
   * Recursive reference for the linked list
   */
  Nfs__Mount__MountList *nextentry;
};
#define NFS__MOUNT__MOUNT_LIST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__mount_list__descriptor) \
    , NULL, NULL, NULL }


/*
 * The "groups" type, a linked list of group names
 */
struct  _Nfs__Mount__Groups
{
  ProtobufCMessage base;
  Nfs__Mount__Name *grname;
  /*
   * Recursive reference for the linked list
   */
  Nfs__Mount__Groups *grnext;
};
#define NFS__MOUNT__GROUPS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__groups__descriptor) \
    , NULL, NULL }


/*
 * The "exportlist" type, a linked list of exported filesystems
 */
struct  _Nfs__Mount__ExportList
{
  ProtobufCMessage base;
  Nfs__Mount__DirPath *filesys;
  Nfs__Mount__Groups *groups;
  /*
   * Recursive reference for the linked list
   */
  Nfs__Mount__ExportList *next;
};
#define NFS__MOUNT__EXPORT_LIST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nfs__mount__export_list__descriptor) \
    , NULL, NULL, NULL }


/* Nfs__Mount__Empty methods */
void   nfs__mount__empty__init
                     (Nfs__Mount__Empty         *message);
size_t nfs__mount__empty__get_packed_size
                     (const Nfs__Mount__Empty   *message);
size_t nfs__mount__empty__pack
                     (const Nfs__Mount__Empty   *message,
                      uint8_t             *out);
size_t nfs__mount__empty__pack_to_buffer
                     (const Nfs__Mount__Empty   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__Empty *
       nfs__mount__empty__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__empty__free_unpacked
                     (Nfs__Mount__Empty *message,
                      ProtobufCAllocator *allocator);
/* Nfs__Mount__FHandle methods */
void   nfs__mount__fhandle__init
                     (Nfs__Mount__FHandle         *message);
size_t nfs__mount__fhandle__get_packed_size
                     (const Nfs__Mount__FHandle   *message);
size_t nfs__mount__fhandle__pack
                     (const Nfs__Mount__FHandle   *message,
                      uint8_t             *out);
size_t nfs__mount__fhandle__pack_to_buffer
                     (const Nfs__Mount__FHandle   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__FHandle *
       nfs__mount__fhandle__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__fhandle__free_unpacked
                     (Nfs__Mount__FHandle *message,
                      ProtobufCAllocator *allocator);
/* Nfs__Mount__FhStatus methods */
void   nfs__mount__fh_status__init
                     (Nfs__Mount__FhStatus         *message);
size_t nfs__mount__fh_status__get_packed_size
                     (const Nfs__Mount__FhStatus   *message);
size_t nfs__mount__fh_status__pack
                     (const Nfs__Mount__FhStatus   *message,
                      uint8_t             *out);
size_t nfs__mount__fh_status__pack_to_buffer
                     (const Nfs__Mount__FhStatus   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__FhStatus *
       nfs__mount__fh_status__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__fh_status__free_unpacked
                     (Nfs__Mount__FhStatus *message,
                      ProtobufCAllocator *allocator);
/* Nfs__Mount__DirPath methods */
void   nfs__mount__dir_path__init
                     (Nfs__Mount__DirPath         *message);
size_t nfs__mount__dir_path__get_packed_size
                     (const Nfs__Mount__DirPath   *message);
size_t nfs__mount__dir_path__pack
                     (const Nfs__Mount__DirPath   *message,
                      uint8_t             *out);
size_t nfs__mount__dir_path__pack_to_buffer
                     (const Nfs__Mount__DirPath   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__DirPath *
       nfs__mount__dir_path__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__dir_path__free_unpacked
                     (Nfs__Mount__DirPath *message,
                      ProtobufCAllocator *allocator);
/* Nfs__Mount__Name methods */
void   nfs__mount__name__init
                     (Nfs__Mount__Name         *message);
size_t nfs__mount__name__get_packed_size
                     (const Nfs__Mount__Name   *message);
size_t nfs__mount__name__pack
                     (const Nfs__Mount__Name   *message,
                      uint8_t             *out);
size_t nfs__mount__name__pack_to_buffer
                     (const Nfs__Mount__Name   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__Name *
       nfs__mount__name__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__name__free_unpacked
                     (Nfs__Mount__Name *message,
                      ProtobufCAllocator *allocator);
/* Nfs__Mount__MountList methods */
void   nfs__mount__mount_list__init
                     (Nfs__Mount__MountList         *message);
size_t nfs__mount__mount_list__get_packed_size
                     (const Nfs__Mount__MountList   *message);
size_t nfs__mount__mount_list__pack
                     (const Nfs__Mount__MountList   *message,
                      uint8_t             *out);
size_t nfs__mount__mount_list__pack_to_buffer
                     (const Nfs__Mount__MountList   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__MountList *
       nfs__mount__mount_list__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__mount_list__free_unpacked
                     (Nfs__Mount__MountList *message,
                      ProtobufCAllocator *allocator);
/* Nfs__Mount__Groups methods */
void   nfs__mount__groups__init
                     (Nfs__Mount__Groups         *message);
size_t nfs__mount__groups__get_packed_size
                     (const Nfs__Mount__Groups   *message);
size_t nfs__mount__groups__pack
                     (const Nfs__Mount__Groups   *message,
                      uint8_t             *out);
size_t nfs__mount__groups__pack_to_buffer
                     (const Nfs__Mount__Groups   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__Groups *
       nfs__mount__groups__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__groups__free_unpacked
                     (Nfs__Mount__Groups *message,
                      ProtobufCAllocator *allocator);
/* Nfs__Mount__ExportList methods */
void   nfs__mount__export_list__init
                     (Nfs__Mount__ExportList         *message);
size_t nfs__mount__export_list__get_packed_size
                     (const Nfs__Mount__ExportList   *message);
size_t nfs__mount__export_list__pack
                     (const Nfs__Mount__ExportList   *message,
                      uint8_t             *out);
size_t nfs__mount__export_list__pack_to_buffer
                     (const Nfs__Mount__ExportList   *message,
                      ProtobufCBuffer     *buffer);
Nfs__Mount__ExportList *
       nfs__mount__export_list__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nfs__mount__export_list__free_unpacked
                     (Nfs__Mount__ExportList *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Nfs__Mount__Empty_Closure)
                 (const Nfs__Mount__Empty *message,
                  void *closure_data);
typedef void (*Nfs__Mount__FHandle_Closure)
                 (const Nfs__Mount__FHandle *message,
                  void *closure_data);
typedef void (*Nfs__Mount__FhStatus_Closure)
                 (const Nfs__Mount__FhStatus *message,
                  void *closure_data);
typedef void (*Nfs__Mount__DirPath_Closure)
                 (const Nfs__Mount__DirPath *message,
                  void *closure_data);
typedef void (*Nfs__Mount__Name_Closure)
                 (const Nfs__Mount__Name *message,
                  void *closure_data);
typedef void (*Nfs__Mount__MountList_Closure)
                 (const Nfs__Mount__MountList *message,
                  void *closure_data);
typedef void (*Nfs__Mount__Groups_Closure)
                 (const Nfs__Mount__Groups *message,
                  void *closure_data);
typedef void (*Nfs__Mount__ExportList_Closure)
                 (const Nfs__Mount__ExportList *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor nfs__mount__empty__descriptor;
extern const ProtobufCMessageDescriptor nfs__mount__fhandle__descriptor;
extern const ProtobufCMessageDescriptor nfs__mount__fh_status__descriptor;
extern const ProtobufCMessageDescriptor nfs__mount__dir_path__descriptor;
extern const ProtobufCMessageDescriptor nfs__mount__name__descriptor;
extern const ProtobufCMessageDescriptor nfs__mount__mount_list__descriptor;
extern const ProtobufCMessageDescriptor nfs__mount__groups__descriptor;
extern const ProtobufCMessageDescriptor nfs__mount__export_list__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_serialization_2fmount_2fmount_2eproto__INCLUDED */
