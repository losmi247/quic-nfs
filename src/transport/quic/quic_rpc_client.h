#ifndef quic_client__header__INCLUDED
#define quic_client__header__INCLUDED

#define _OPEN_SYS_ITOA_EXT // so we can use utoa()

#include <errno.h>
#include <ev.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "openssl/ssl.h"
#include "tquic.h"

#include "quic_client.h"
#include "quic_record_marking.h"

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/common_rpc/common_rpc.h"
#include "src/common_rpc/rpc_connection_context.h"

#define MAX_DATAGRAM_SIZE 10000

/*
 * QUIC methods
 */

extern struct quic_transport_methods_t quic_transport_methods;

extern struct quic_packet_send_methods_t quic_packet_send_methods;

/*
 * RPC functions over QUIC
 */

Rpc__RpcMsg *invoke_rpc_remote_quic(RpcConnectionContext *rpc_connection_context, uint32_t program_number,
                                    uint32_t program_version, uint32_t procedure_number,
                                    Google__Protobuf__Any parameters, bool use_auxiliary_stream);

void async_connection_closing_callback(EV_P_ ev_async *w, int revents);

#endif /* quic_client__header__INCLUDED */