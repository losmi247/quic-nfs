#ifndef tcp_rpc_server__header__INCLUDED
#define tcp_rpc_server__header__INCLUDED

#include <pthread.h>
#include <stdbool.h>

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/common_rpc/server_common_rpc.h"
#include "src/common_rpc/common_rpc.h"

#include "src/transport/tcp/tcp_record_marking.h"

#define TCP_RCVBUF_SIZE 65536
#define TCP_SNDBUF_SIZE 65536

int run_server_tcp(uint16_t port_number);

/*
* TCP Nfs+Mount server state.
*/

extern int rpc_server_socket_fd;

extern pthread_mutex_t tcp_server_cleanup_mutex;
extern bool tcp_server_resources_released;

void clean_up_tcp_server_state(void);

#endif /* tcp_rpc_server__header__INCLUDED */