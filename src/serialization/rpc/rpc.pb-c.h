/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: src/serialization/rpc/rpc.proto */

#ifndef PROTOBUF_C_src_2fserialization_2frpc_2frpc_2eproto__INCLUDED
#define PROTOBUF_C_src_2fserialization_2frpc_2frpc_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif

#include "src/serialization/google_protos/any.pb-c.h"
#include "src/serialization/google_protos/empty.pb-c.h"

typedef struct _Rpc__CallBody Rpc__CallBody;
typedef struct _Rpc__MismatchInfo Rpc__MismatchInfo;
typedef struct _Rpc__AcceptedReply Rpc__AcceptedReply;
typedef struct _Rpc__RejectedReply Rpc__RejectedReply;
typedef struct _Rpc__ReplyBody Rpc__ReplyBody;
typedef struct _Rpc__RpcMsg Rpc__RpcMsg;


/* --- enums --- */

/*
 * Message types
 */
typedef enum _Rpc__MsgType {
  RPC__MSG_TYPE__CALL = 0,
  RPC__MSG_TYPE__REPLY = 1
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__MSG_TYPE)
} Rpc__MsgType;
/*
 * Reply status
 */
typedef enum _Rpc__ReplyStat {
  RPC__REPLY_STAT__MSG_ACCEPTED = 0,
  RPC__REPLY_STAT__MSG_DENIED = 1
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__REPLY_STAT)
} Rpc__ReplyStat;
/*
 * Accept status
 */
typedef enum _Rpc__AcceptStat {
  /*
   * RPC executed successfully
   */
  RPC__ACCEPT_STAT__SUCCESS = 0,
  /*
   * remote hasn't exported program
   */
  RPC__ACCEPT_STAT__PROG_UNAVAIL = 1,
  /*
   * remote can't support version
   */
  RPC__ACCEPT_STAT__PROG_MISMATCH = 2,
  /*
   * program can't support procedure
   */
  RPC__ACCEPT_STAT__PROC_UNAVAIL = 3,
  /*
   * procedure can't decode params
   */
  RPC__ACCEPT_STAT__GARBAGE_ARGS = 4,
  /*
   * memory allocation failure
   */
  RPC__ACCEPT_STAT__SYSTEM_ERR = 5
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__ACCEPT_STAT)
} Rpc__AcceptStat;
/*
 * Reject status
 */
typedef enum _Rpc__RejectStat {
  /*
   * RPC version number != 2
   */
  RPC__REJECT_STAT__RPC_MISMATCH = 0,
  /*
   * remote can't authenticate caller
   */
  RPC__REJECT_STAT__AUTH_ERROR = 1
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__REJECT_STAT)
} Rpc__RejectStat;
/*
 * Auth status
 */
typedef enum _Rpc__AuthStat {
  RPC__AUTH_STAT__AUTH_OK = 0
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__AUTH_STAT)
} Rpc__AuthStat;

/* --- messages --- */

/*
 * Generic CallBody to handle any program-specific procedures
 */
struct  _Rpc__CallBody
{
  ProtobufCMessage base;
  /*
   * RPC version number (must be 2)
   */
  uint32_t rpcvers;
  /*
   * RPC program number
   */
  uint32_t prog;
  /*
   * version number of the program
   */
  uint32_t vers;
  /*
   * procedure number
   */
  uint32_t proc;
  /*
   * Procedure-specific parameters can be added here as fields
   */
  /*
   * Serialized params for the procedure
   */
  Google__Protobuf__Any *params;
};
#define RPC__CALL_BODY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&rpc__call_body__descriptor) \
    , 0, 0, 0, 0, NULL }


/*
 * MismatchInfo message definition (for AcceptedReply and RejectedReply)
 */
struct  _Rpc__MismatchInfo
{
  ProtobufCMessage base;
  uint32_t low;
  uint32_t high;
};
#define RPC__MISMATCH_INFO__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&rpc__mismatch_info__descriptor) \
    , 0, 0 }


typedef enum {
  RPC__ACCEPTED_REPLY__REPLY_DATA__NOT_SET = 0,
  RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS = 2,
  RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO = 3,
  RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE = 4
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__ACCEPTED_REPLY__REPLY_DATA)
} Rpc__AcceptedReply__ReplyDataCase;

/*
 * AcceptedReply message definition
 */
struct  _Rpc__AcceptedReply
{
  ProtobufCMessage base;
  /*
   * here: opaque verf for auth protocol - length 0 when AUTH_NONE
   */
  Rpc__AcceptStat stat;
  Rpc__AcceptedReply__ReplyDataCase reply_data_case;
  union {
    /*
     * case SUCCESS
     */
    Google__Protobuf__Any *results;
    /*
     * case PROG_MISMATCH
     */
    Rpc__MismatchInfo *mismatch_info;
    /*
     * default case
     */
    Google__Protobuf__Empty *default_case;
  };
};
#define RPC__ACCEPTED_REPLY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&rpc__accepted_reply__descriptor) \
    , RPC__ACCEPT_STAT__SUCCESS, RPC__ACCEPTED_REPLY__REPLY_DATA__NOT_SET, {0} }


typedef enum {
  RPC__REJECTED_REPLY__REPLY_DATA__NOT_SET = 0,
  RPC__REJECTED_REPLY__REPLY_DATA_MISMATCH_INFO = 2,
  RPC__REJECTED_REPLY__REPLY_DATA_AUTH_STAT = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__REJECTED_REPLY__REPLY_DATA)
} Rpc__RejectedReply__ReplyDataCase;

/*
 * RejectedReply message definition
 */
struct  _Rpc__RejectedReply
{
  ProtobufCMessage base;
  Rpc__RejectStat stat;
  Rpc__RejectedReply__ReplyDataCase reply_data_case;
  union {
    /*
     * case RPC_MISMATCH
     */
    Rpc__MismatchInfo *mismatch_info;
    /*
     * case AUTH_ERROR
     */
    Rpc__AuthStat auth_stat;
  };
};
#define RPC__REJECTED_REPLY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&rpc__rejected_reply__descriptor) \
    , RPC__REJECT_STAT__RPC_MISMATCH, RPC__REJECTED_REPLY__REPLY_DATA__NOT_SET, {0} }


typedef enum {
  RPC__REPLY_BODY__REPLY__NOT_SET = 0,
  RPC__REPLY_BODY__REPLY_AREPLY = 2,
  RPC__REPLY_BODY__REPLY_RREPLY = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__REPLY_BODY__REPLY)
} Rpc__ReplyBody__ReplyCase;

/*
 * ReplyBody message definition
 */
struct  _Rpc__ReplyBody
{
  ProtobufCMessage base;
  Rpc__ReplyStat stat;
  Rpc__ReplyBody__ReplyCase reply_case;
  union {
    /*
     * case MSG_ACCEPTED
     */
    Rpc__AcceptedReply *areply;
    /*
     * case MSG_DENIED
     */
    Rpc__RejectedReply *rreply;
  };
};
#define RPC__REPLY_BODY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&rpc__reply_body__descriptor) \
    , RPC__REPLY_STAT__MSG_ACCEPTED, RPC__REPLY_BODY__REPLY__NOT_SET, {0} }


typedef enum {
  RPC__RPC_MSG__BODY__NOT_SET = 0,
  RPC__RPC_MSG__BODY_CBODY = 3,
  RPC__RPC_MSG__BODY_RBODY = 4
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(RPC__RPC_MSG__BODY)
} Rpc__RpcMsg__BodyCase;

/*
 * RpcMsg message definition
 */
struct  _Rpc__RpcMsg
{
  ProtobufCMessage base;
  uint32_t xid;
  Rpc__MsgType mtype;
  Rpc__RpcMsg__BodyCase body_case;
  union {
    /*
     * case CALL
     */
    Rpc__CallBody *cbody;
    /*
     * case REPLY
     */
    Rpc__ReplyBody *rbody;
  };
};
#define RPC__RPC_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&rpc__rpc_msg__descriptor) \
    , 0, RPC__MSG_TYPE__CALL, RPC__RPC_MSG__BODY__NOT_SET, {0} }


/* Rpc__CallBody methods */
void   rpc__call_body__init
                     (Rpc__CallBody         *message);
size_t rpc__call_body__get_packed_size
                     (const Rpc__CallBody   *message);
size_t rpc__call_body__pack
                     (const Rpc__CallBody   *message,
                      uint8_t             *out);
size_t rpc__call_body__pack_to_buffer
                     (const Rpc__CallBody   *message,
                      ProtobufCBuffer     *buffer);
Rpc__CallBody *
       rpc__call_body__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   rpc__call_body__free_unpacked
                     (Rpc__CallBody *message,
                      ProtobufCAllocator *allocator);
/* Rpc__MismatchInfo methods */
void   rpc__mismatch_info__init
                     (Rpc__MismatchInfo         *message);
size_t rpc__mismatch_info__get_packed_size
                     (const Rpc__MismatchInfo   *message);
size_t rpc__mismatch_info__pack
                     (const Rpc__MismatchInfo   *message,
                      uint8_t             *out);
size_t rpc__mismatch_info__pack_to_buffer
                     (const Rpc__MismatchInfo   *message,
                      ProtobufCBuffer     *buffer);
Rpc__MismatchInfo *
       rpc__mismatch_info__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   rpc__mismatch_info__free_unpacked
                     (Rpc__MismatchInfo *message,
                      ProtobufCAllocator *allocator);
/* Rpc__AcceptedReply methods */
void   rpc__accepted_reply__init
                     (Rpc__AcceptedReply         *message);
size_t rpc__accepted_reply__get_packed_size
                     (const Rpc__AcceptedReply   *message);
size_t rpc__accepted_reply__pack
                     (const Rpc__AcceptedReply   *message,
                      uint8_t             *out);
size_t rpc__accepted_reply__pack_to_buffer
                     (const Rpc__AcceptedReply   *message,
                      ProtobufCBuffer     *buffer);
Rpc__AcceptedReply *
       rpc__accepted_reply__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   rpc__accepted_reply__free_unpacked
                     (Rpc__AcceptedReply *message,
                      ProtobufCAllocator *allocator);
/* Rpc__RejectedReply methods */
void   rpc__rejected_reply__init
                     (Rpc__RejectedReply         *message);
size_t rpc__rejected_reply__get_packed_size
                     (const Rpc__RejectedReply   *message);
size_t rpc__rejected_reply__pack
                     (const Rpc__RejectedReply   *message,
                      uint8_t             *out);
size_t rpc__rejected_reply__pack_to_buffer
                     (const Rpc__RejectedReply   *message,
                      ProtobufCBuffer     *buffer);
Rpc__RejectedReply *
       rpc__rejected_reply__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   rpc__rejected_reply__free_unpacked
                     (Rpc__RejectedReply *message,
                      ProtobufCAllocator *allocator);
/* Rpc__ReplyBody methods */
void   rpc__reply_body__init
                     (Rpc__ReplyBody         *message);
size_t rpc__reply_body__get_packed_size
                     (const Rpc__ReplyBody   *message);
size_t rpc__reply_body__pack
                     (const Rpc__ReplyBody   *message,
                      uint8_t             *out);
size_t rpc__reply_body__pack_to_buffer
                     (const Rpc__ReplyBody   *message,
                      ProtobufCBuffer     *buffer);
Rpc__ReplyBody *
       rpc__reply_body__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   rpc__reply_body__free_unpacked
                     (Rpc__ReplyBody *message,
                      ProtobufCAllocator *allocator);
/* Rpc__RpcMsg methods */
void   rpc__rpc_msg__init
                     (Rpc__RpcMsg         *message);
size_t rpc__rpc_msg__get_packed_size
                     (const Rpc__RpcMsg   *message);
size_t rpc__rpc_msg__pack
                     (const Rpc__RpcMsg   *message,
                      uint8_t             *out);
size_t rpc__rpc_msg__pack_to_buffer
                     (const Rpc__RpcMsg   *message,
                      ProtobufCBuffer     *buffer);
Rpc__RpcMsg *
       rpc__rpc_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   rpc__rpc_msg__free_unpacked
                     (Rpc__RpcMsg *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Rpc__CallBody_Closure)
                 (const Rpc__CallBody *message,
                  void *closure_data);
typedef void (*Rpc__MismatchInfo_Closure)
                 (const Rpc__MismatchInfo *message,
                  void *closure_data);
typedef void (*Rpc__AcceptedReply_Closure)
                 (const Rpc__AcceptedReply *message,
                  void *closure_data);
typedef void (*Rpc__RejectedReply_Closure)
                 (const Rpc__RejectedReply *message,
                  void *closure_data);
typedef void (*Rpc__ReplyBody_Closure)
                 (const Rpc__ReplyBody *message,
                  void *closure_data);
typedef void (*Rpc__RpcMsg_Closure)
                 (const Rpc__RpcMsg *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    rpc__msg_type__descriptor;
extern const ProtobufCEnumDescriptor    rpc__reply_stat__descriptor;
extern const ProtobufCEnumDescriptor    rpc__accept_stat__descriptor;
extern const ProtobufCEnumDescriptor    rpc__reject_stat__descriptor;
extern const ProtobufCEnumDescriptor    rpc__auth_stat__descriptor;
extern const ProtobufCMessageDescriptor rpc__call_body__descriptor;
extern const ProtobufCMessageDescriptor rpc__mismatch_info__descriptor;
extern const ProtobufCMessageDescriptor rpc__accepted_reply__descriptor;
extern const ProtobufCMessageDescriptor rpc__rejected_reply__descriptor;
extern const ProtobufCMessageDescriptor rpc__reply_body__descriptor;
extern const ProtobufCMessageDescriptor rpc__rpc_msg__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_src_2fserialization_2frpc_2frpc_2eproto__INCLUDED */