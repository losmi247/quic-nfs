#ifndef tcp_client__HEADER__INCLUDED
#define tcp_client__HEADER__INCLUDED

#include "pthread.h"

typedef struct TcpClient {
    int *tcp_rpc_client_socket_fd;
    pthread_mutex_t tcp_connection_mutex;
} TcpClient;

#endif /* tcp_client__HEADER__INCLUDED */