#ifndef transport_common__header__INCLUDED
#define transport_common__header__INCLUDED

#include "src/transport/quic/quic_client.h"
#include "src/transport/tcp/tcp_client.h"

#define RM_FRAGMENT_HEADER_SIZE 4            // RPC Record Marking fragment header size in bytes
#define RM_MAX_FRAGMENT_DATA_SIZE 0x7FFFFFFF // max size in bytes of the data in a Record Marking fragment

typedef enum TransportProtocol {
    TRANSPORT_PROTOCOL_TCP = 0,
    TRANSPORT_PROTOCOL_QUIC = 1
} TransportProtocol;

typedef union {
    // TCP
    TcpClient *tcp_client;

    // QUIC
    QuicClient *quic_client;
} TransportConnection;

#endif /* transport_common__header__INCLUDED */