#ifndef nfs_server_threads__HEADER__INCLUDED
#define nfs_server_threads__HEADER__INCLUDED

#include <pthread.h>

#include "src/transport/transport_common.h"

typedef struct NfsServerThreadsList {
    pthread_t server_thread;

    TransportProtocol transport_protocol;
    TransportConnection transport_connection;

    struct NfsServerThreadsList *next;
} NfsServerThreadsList;

int add_server_thread(pthread_t server_thread, TransportProtocol transport_protocol, TransportConnection transport_connection, NfsServerThreadsList **head);

int remove_server_thread(pthread_t server_thread, NfsServerThreadsList **head);

void clean_up_nfs_server_threads_list(NfsServerThreadsList *nfs_server_threads_list);

#endif /* nfs_server_threads__HEADER__INCLUDED */