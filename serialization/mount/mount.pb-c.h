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

#include "serialization/google_protos/empty.pb-c.h"

typedef struct _Mount__FHandle Mount__FHandle;
typedef struct _Mount__FhStatus Mount__FhStatus;
typedef struct _Mount__DirPath Mount__DirPath;
typedef struct _Mount__Name Mount__Name;
typedef struct _Mount__MountList Mount__MountList;
typedef struct _Mount__Groups Mount__Groups;
typedef struct _Mount__ExportList Mount__ExportList;


/* --- enums --- */


/* --- messages --- */

/*
 * File Handle
 */
struct  _Mount__FHandle
{
  ProtobufCMessage base;
  ProtobufCBinaryData handle;
};
#define MOUNT__FHANDLE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mount__fhandle__descriptor) \
    , {0,NULL} }


typedef enum {
  MOUNT__FH_STATUS__FHSTATUS_BODY__NOT_SET = 0,
  MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY = 2,
  MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(MOUNT__FH_STATUS__FHSTATUS_BODY)
} Mount__FhStatus__FhstatusBodyCase;

struct  _Mount__FhStatus
{
  ProtobufCMessage base;
  /*
   * 0 for success, non-zero for error
   */
  uint32_t status;
  Mount__FhStatus__FhstatusBodyCase fhstatus_body_case;
  union {
    /*
     * case status is 0
     */
    Mount__FHandle *directory;
    /*
     * default case (status > 0)
     */
    Google__Protobuf__Empty *default_case;
  };
};
#define MOUNT__FH_STATUS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mount__fh_status__descriptor) \
    , 0, MOUNT__FH_STATUS__FHSTATUS_BODY__NOT_SET, {0} }


/*
 * A string representing a server pathname of directory
 */
struct  _Mount__DirPath
{
  ProtobufCMessage base;
  char *path;
};
#define MOUNT__DIR_PATH__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mount__dir_path__descriptor) \
    , (char *)protobuf_c_empty_string }


/*
 * An arbitrary string used for hostnames and group names
 */
struct  _Mount__Name
{
  ProtobufCMessage base;
  char *name;
};
#define MOUNT__NAME__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mount__name__descriptor) \
    , (char *)protobuf_c_empty_string }


/*
 * A linked list of mounted entries
 */
struct  _Mount__MountList
{
  ProtobufCMessage base;
  Mount__Name *hostname;
  Mount__DirPath *directory;
  /*
   * Recursive reference for the linked list
   */
  Mount__MountList *nextentry;
};
#define MOUNT__MOUNT_LIST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mount__mount_list__descriptor) \
    , NULL, NULL, NULL }


/*
 * The "groups" type, a linked list of group names
 */
struct  _Mount__Groups
{
  ProtobufCMessage base;
  Mount__Name *grname;
  /*
   * Recursive reference for the linked list
   */
  Mount__Groups *grnext;
};
#define MOUNT__GROUPS__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mount__groups__descriptor) \
    , NULL, NULL }


/*
 * The "exportlist" type, a linked list of exported filesystems
 */
struct  _Mount__ExportList
{
  ProtobufCMessage base;
  Mount__DirPath *filesys;
  Mount__Groups *groups;
  /*
   * Recursive reference for the linked list
   */
  Mount__ExportList *next;
};
#define MOUNT__EXPORT_LIST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mount__export_list__descriptor) \
    , NULL, NULL, NULL }


/* Mount__FHandle methods */
void   mount__fhandle__init
                     (Mount__FHandle         *message);
size_t mount__fhandle__get_packed_size
                     (const Mount__FHandle   *message);
size_t mount__fhandle__pack
                     (const Mount__FHandle   *message,
                      uint8_t             *out);
size_t mount__fhandle__pack_to_buffer
                     (const Mount__FHandle   *message,
                      ProtobufCBuffer     *buffer);
Mount__FHandle *
       mount__fhandle__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mount__fhandle__free_unpacked
                     (Mount__FHandle *message,
                      ProtobufCAllocator *allocator);
/* Mount__FhStatus methods */
void   mount__fh_status__init
                     (Mount__FhStatus         *message);
size_t mount__fh_status__get_packed_size
                     (const Mount__FhStatus   *message);
size_t mount__fh_status__pack
                     (const Mount__FhStatus   *message,
                      uint8_t             *out);
size_t mount__fh_status__pack_to_buffer
                     (const Mount__FhStatus   *message,
                      ProtobufCBuffer     *buffer);
Mount__FhStatus *
       mount__fh_status__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mount__fh_status__free_unpacked
                     (Mount__FhStatus *message,
                      ProtobufCAllocator *allocator);
/* Mount__DirPath methods */
void   mount__dir_path__init
                     (Mount__DirPath         *message);
size_t mount__dir_path__get_packed_size
                     (const Mount__DirPath   *message);
size_t mount__dir_path__pack
                     (const Mount__DirPath   *message,
                      uint8_t             *out);
size_t mount__dir_path__pack_to_buffer
                     (const Mount__DirPath   *message,
                      ProtobufCBuffer     *buffer);
Mount__DirPath *
       mount__dir_path__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mount__dir_path__free_unpacked
                     (Mount__DirPath *message,
                      ProtobufCAllocator *allocator);
/* Mount__Name methods */
void   mount__name__init
                     (Mount__Name         *message);
size_t mount__name__get_packed_size
                     (const Mount__Name   *message);
size_t mount__name__pack
                     (const Mount__Name   *message,
                      uint8_t             *out);
size_t mount__name__pack_to_buffer
                     (const Mount__Name   *message,
                      ProtobufCBuffer     *buffer);
Mount__Name *
       mount__name__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mount__name__free_unpacked
                     (Mount__Name *message,
                      ProtobufCAllocator *allocator);
/* Mount__MountList methods */
void   mount__mount_list__init
                     (Mount__MountList         *message);
size_t mount__mount_list__get_packed_size
                     (const Mount__MountList   *message);
size_t mount__mount_list__pack
                     (const Mount__MountList   *message,
                      uint8_t             *out);
size_t mount__mount_list__pack_to_buffer
                     (const Mount__MountList   *message,
                      ProtobufCBuffer     *buffer);
Mount__MountList *
       mount__mount_list__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mount__mount_list__free_unpacked
                     (Mount__MountList *message,
                      ProtobufCAllocator *allocator);
/* Mount__Groups methods */
void   mount__groups__init
                     (Mount__Groups         *message);
size_t mount__groups__get_packed_size
                     (const Mount__Groups   *message);
size_t mount__groups__pack
                     (const Mount__Groups   *message,
                      uint8_t             *out);
size_t mount__groups__pack_to_buffer
                     (const Mount__Groups   *message,
                      ProtobufCBuffer     *buffer);
Mount__Groups *
       mount__groups__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mount__groups__free_unpacked
                     (Mount__Groups *message,
                      ProtobufCAllocator *allocator);
/* Mount__ExportList methods */
void   mount__export_list__init
                     (Mount__ExportList         *message);
size_t mount__export_list__get_packed_size
                     (const Mount__ExportList   *message);
size_t mount__export_list__pack
                     (const Mount__ExportList   *message,
                      uint8_t             *out);
size_t mount__export_list__pack_to_buffer
                     (const Mount__ExportList   *message,
                      ProtobufCBuffer     *buffer);
Mount__ExportList *
       mount__export_list__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mount__export_list__free_unpacked
                     (Mount__ExportList *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Mount__FHandle_Closure)
                 (const Mount__FHandle *message,
                  void *closure_data);
typedef void (*Mount__FhStatus_Closure)
                 (const Mount__FhStatus *message,
                  void *closure_data);
typedef void (*Mount__DirPath_Closure)
                 (const Mount__DirPath *message,
                  void *closure_data);
typedef void (*Mount__Name_Closure)
                 (const Mount__Name *message,
                  void *closure_data);
typedef void (*Mount__MountList_Closure)
                 (const Mount__MountList *message,
                  void *closure_data);
typedef void (*Mount__Groups_Closure)
                 (const Mount__Groups *message,
                  void *closure_data);
typedef void (*Mount__ExportList_Closure)
                 (const Mount__ExportList *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor mount__fhandle__descriptor;
extern const ProtobufCMessageDescriptor mount__fh_status__descriptor;
extern const ProtobufCMessageDescriptor mount__dir_path__descriptor;
extern const ProtobufCMessageDescriptor mount__name__descriptor;
extern const ProtobufCMessageDescriptor mount__mount_list__descriptor;
extern const ProtobufCMessageDescriptor mount__groups__descriptor;
extern const ProtobufCMessageDescriptor mount__export_list__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_serialization_2fmount_2fmount_2eproto__INCLUDED */
