#ifndef quic_rpc_server__header__INCLUDED
#define quic_rpc_server__header__INCLUDED

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

#include <pthread.h>

#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/x509.h"
#include "tquic.h"

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/common_rpc/common_rpc.h"
#include "src/common_rpc/server_common_rpc.h"

#include "src/transport/quic/quic_record_marking.h"

#define MAX_DATAGRAM_SIZE 10000

struct QuicServer {
    struct quic_endpoint_t *quic_endpoint;

    // underlying UDP socket
    int socket_fd;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;

    struct quic_config_t *config;
    struct quic_tls_config_t *tls_config;

    struct ev_loop *event_loop;
    ev_timer timer;

    // RPC messages being received as Record Marking records
    RecordMarkingReceivingContextsList *rm_receiving_contexts;
};

int run_server_quic(uint16_t port_number);

/*
 * QUIC Nfs+Mount server state.
 */

extern struct QuicServer quic_server;

extern pthread_mutex_t quic_server_cleanup_mutex;
extern bool quic_server_resources_released;

void clean_up_quic_server_state(void);

#endif /* quic_rpc_server__header__INCLUDED */