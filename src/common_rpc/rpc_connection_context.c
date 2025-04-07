#include "rpc_connection_context.h"

#include <arpa/inet.h>
#include <ev.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "src/transport/tcp/tcp_rpc_server.h"

#include "src/transport/quic/quic_rpc_client.h"

/*
 * Given a RpcConnectionContext without an initialized TCP client socket,
 * creates a TCP client socket and connects it to the server given by its IPv4
 * address and port in the RpcConnectionContext, and saves the client TCP socket
 * in the given RpcConnectionContext.
 *
 * The user of this function takes the responsibility to close the client TCP
 * socket opened here.
 *
 * Returns 0 on success and > 0 on failure.
 */
int connect_to_tcp_server(RpcConnectionContext *rpc_connection_context) {
    if (rpc_connection_context == NULL) {
        return 1;
    }

    if (rpc_connection_context->server_ipv4_addr == NULL) {
        return 2;
    }

    int *tcp_rpc_client_socket_fd = malloc(sizeof(int));
    if (tcp_rpc_client_socket_fd == NULL) {
        return 3;
    }
    *tcp_rpc_client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*tcp_rpc_client_socket_fd < 0) {
        perror_msg("connect_to_tcp_server: Socket creation failed");
        return 4;
    }

    // disable Nagle's algorithm - send TCP segments as soon as they are available
    int flag = 1;
    setsockopt(*tcp_rpc_client_socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

    int sndbuf_size = TCP_SNDBUF_SIZE;
    setsockopt(*tcp_rpc_client_socket_fd, SOL_SOCKET, SO_RCVBUF, &sndbuf_size, sizeof(sndbuf_size));
    int rcvbuf_size = TCP_RCVBUF_SIZE;
    setsockopt(*tcp_rpc_client_socket_fd, SOL_SOCKET, SO_SNDBUF, &rcvbuf_size, sizeof(rcvbuf_size));

    TransportConnection *transport_connection = malloc(sizeof(TransportConnection));
    if (transport_connection == NULL) {
        return 5;
    }
    transport_connection->tcp_rpc_client_socket_fd = tcp_rpc_client_socket_fd;
    rpc_connection_context->transport_connection = transport_connection;

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET;
    rpc_server_addr.sin_addr.s_addr = inet_addr(rpc_connection_context->server_ipv4_addr);
    rpc_server_addr.sin_port = htons(rpc_connection_context->server_port);

    // connect the rpc client socket to rpc server socket
    if (connect(*rpc_connection_context->transport_connection->tcp_rpc_client_socket_fd,
                (struct sockaddr *)&rpc_server_addr, sizeof(rpc_server_addr)) < 0) {
        perror_msg("Connection to the server failed");

        close(*rpc_connection_context->transport_connection->tcp_rpc_client_socket_fd);
        free(rpc_connection_context->transport_connection->tcp_rpc_client_socket_fd);

        return 6;
    }

    return 0;
}

/*
 * Creates an underlying UDP socket which can be used for sending QUIC packets.
 *
 * Returns 0 on success and > 0 on failure.
 */
int create_quic_udp_socket(char *host, char *port, struct addrinfo **peer, QuicClient *quic_client) {
    const struct addrinfo hints = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
    if (getaddrinfo(host, port, &hints, peer) != 0) {
        fprintf(stderr, "create_socket: failed to resolve host\n");
        return 1;
    }

    int udp_socket = socket((*peer)->ai_family, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        fprintf(stderr, "create_socket: failed to create socket\n");
        return 1;
    }
    if (fcntl(udp_socket, F_SETFL, O_NONBLOCK) != 0) {
        fprintf(stderr, "create_socket: failed to make socket non-blocking\n");
        return 1;
    }

    quic_client->local_addr_len = sizeof(quic_client->local_addr);
    if (getsockname(udp_socket, (struct sockaddr *)&quic_client->local_addr, &quic_client->local_addr_len) != 0) {
        fprintf(stderr, "create_socket: failed to get local address of socket\n");
        return 1;
    };
    quic_client->socket_fd = udp_socket;

    return 0;
}

/*
 * Given a RpcConnectionContext without an initialized QUIC connection,
 * creates a QUIC client UDP socket and connects it to the server given by its IPv4
 * address and port in the RpcConnectionContext, and saves that QUIC connection context
 * in the given RpcConnectionContext.
 *
 * The user of this function takes the responsibility to close the QUIC connection
 * opened here.
 *
 * Returns 0 on success and > 0 on failure.
 */
int connect_to_quic_server(RpcConnectionContext *rpc_connection_context) {
    if (rpc_connection_context == NULL) {
        return 1;
    }

    if (rpc_connection_context->server_ipv4_addr == NULL) {
        return 2;
    }

    QuicClient *quic_client = malloc(sizeof(QuicClient));
    if (quic_client == NULL) {
        return 3;
    }
    quic_client->quic_config = NULL;
    quic_client->tls_config = NULL;
    quic_client->quic_endpoint = NULL;
    quic_client->quic_connection = NULL;

    quic_client->event_loop = NULL;
    quic_client->successfully_created_event_loop_thread = false;

    pthread_mutex_init(&quic_client->connection_established_lock, NULL);
    pthread_cond_init(&quic_client->connection_established_condition_variable, NULL);
    quic_client->connection_established = false;

    pthread_mutex_init(&quic_client->connection_closed_lock, NULL);
    pthread_cond_init(&quic_client->connection_closed_condition_variable, NULL);
    quic_client->connection_closed = false;

    quic_client->main_stream = NULL;
    quic_client->auxiliary_streams_list_head = NULL;
    pthread_mutex_init(&quic_client->auxiliary_streams_list_lock, NULL);

    quic_client->stream_contexts = NULL;
    pthread_mutex_init(&quic_client->stream_contexts_list_lock, NULL);
    pthread_cond_init(&quic_client->stream_contexts_condition_variable, NULL);

    int ret = 0;

    // create socket
    struct addrinfo *peer = NULL;
    char port_number[10];
    sprintf(port_number, "%u", rpc_connection_context->server_port);
    if (create_quic_udp_socket(rpc_connection_context->server_ipv4_addr, port_number, &peer, quic_client) != 0) {
        free(quic_client);

        return 4;
    }

    // create QUIC config
    quic_client->quic_config = quic_config_new();
    if (quic_client->quic_config == NULL) {
        fprintf(stderr, "connect_to_quic_server: failed to create config\n");

        free(quic_client);

        return 5;
    }
    quic_config_set_recv_udp_payload_size(quic_client->quic_config, MAX_DATAGRAM_SIZE);

    // create and set TLS config
    const char *const protos[1] = {"rpc"};
    quic_client->tls_config = quic_tls_config_new_client_config(protos, 1, true);
    if (quic_client->tls_config == NULL) {
        fprintf(stderr, "connect_to_quic_server: failed to create TLS config\n");

        free(quic_client);

        quic_config_free(quic_client->quic_config);
        quic_tls_config_free(quic_client->tls_config);

        return 6;
    }
    quic_config_set_tls_config(quic_client->quic_config, quic_client->tls_config);

    // create QUIC endpoint
    quic_client->quic_endpoint = quic_endpoint_new(quic_client->quic_config, false, &quic_transport_methods,
                                                   quic_client, &quic_packet_send_methods, quic_client);
    if (quic_client->quic_endpoint == NULL) {
        fprintf(stderr, "connect_to_quic_server: failed to create quic endpoint\n");

        free(quic_client);

        quic_config_free(quic_client->quic_config);
        quic_tls_config_free(quic_client->tls_config);
        quic_endpoint_free(quic_client->quic_endpoint);

        return 8;
    }

    // save this transport connection in the RPC connection context
    TransportConnection *transport_connection = malloc(sizeof(TransportConnection));
    if (transport_connection == NULL) {
        free(quic_client);

        quic_config_free(quic_client->quic_config);
        quic_tls_config_free(quic_client->tls_config);

        return 10;
    }
    transport_connection->quic_client = quic_client;
    rpc_connection_context->transport_connection = transport_connection;

    // connect to the server
    if (quic_endpoint_connect(quic_client->quic_endpoint, (struct sockaddr *)&quic_client->local_addr,
                              quic_client->local_addr_len, peer->ai_addr, peer->ai_addrlen, NULL, NULL, 0, NULL, 0,
                              NULL, NULL) < 0) {
        fprintf(stderr, "execute_rpc_call_quic: failed to connect to client\n");

        free(quic_client);

        quic_config_free(quic_client->quic_config);
        quic_tls_config_free(quic_client->tls_config);
        quic_endpoint_free(quic_client->quic_endpoint);

        freeaddrinfo(peer);

        return 11;
    }
    freeaddrinfo(peer);

    return 0;
}

/*
 * Creates a RpcConnectionContext with the given server IP address and server port,
 * and the given credential and verifier.
 *
 * The user of this function takes the responsiblity to free the allocated server_ipv4_addr string
 * and the RpcConnectionContext itself.
 *
 * Returns NULL on failure.
 */
RpcConnectionContext *create_rpc_connection_context(char *server_ipv4_address, uint16_t server_port,
                                                    Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier,
                                                    TransportProtocol transport_protocol) {
    RpcConnectionContext *rpc_connection_context = malloc(sizeof(RpcConnectionContext));

    rpc_connection_context->server_ipv4_addr = strdup(server_ipv4_address);
    rpc_connection_context->server_port = server_port;

    rpc_connection_context->credential = credential;
    rpc_connection_context->verifier = verifier;

    int error_code;
    rpc_connection_context->transport_protocol = transport_protocol;
    switch (transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        error_code = connect_to_tcp_server(rpc_connection_context);
        if (error_code > 0) {
            printf("create_rpc_connection_context: Failed to connect to the TCP server with error code %d\n",
                   error_code);

            free(rpc_connection_context->server_ipv4_addr);
            free(rpc_connection_context);

            return NULL;
        }
        break;
    case TRANSPORT_PROTOCOL_QUIC:
        error_code = connect_to_quic_server(rpc_connection_context);
        if (error_code > 0) {
            printf("create_rpc_connection_context: Failed to connect to the QUIC server with error code %d\n",
                   error_code);

            free(rpc_connection_context->server_ipv4_addr);
            free(rpc_connection_context);

            return NULL;
        }
        break;
    default:
        free(rpc_connection_context->server_ipv4_addr);
        free(rpc_connection_context);
        return NULL;
    }

    return rpc_connection_context;
}

/*
 * Creates a RpcConnectionContext with the given server IP address and server port,
 * the credential and verifier both having flavor AUTH_NONE, and the given transport protocol identifier.
 *
 * The user of this function takes the responsiblity to deallocate the received RpcConnectionContext using
 * free_rpc_connection_context() function.
 *
 * Returns NULL on failure.
 */
RpcConnectionContext *create_auth_none_rpc_connection_context(char *server_ipv4_address, uint16_t server_port,
                                                              TransportProtocol transport_protocol) {
    Rpc__OpaqueAuth *credential = create_auth_none_opaque_auth();

    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();

    return create_rpc_connection_context(server_ipv4_address, server_port, credential, verifier, transport_protocol);
}

/*
 * Creates a RpcConnectionContext with the given server IP address and server port,
 * the credential having flavor AUTH_SYS with machine name being the name of the machine on which the process
 * that calls this function runs, uid/gid being the effective uid/gid of the calling process, and the list of
 * groups gids being the list of groups that the calling process belongs too, and the verifier having flavour
 * AUTH_NONE, and the given transport protocol identifier.
 *
 * Returns the constructed RpcConnectionContext on success, and NULL on failure.
 *
 * The user of this function takes the responsiblity to deallocete the received RpcConnectionContext using
 * free_rpc_connection_context() function.
 */
RpcConnectionContext *create_auth_sys_rpc_connection_context(char *server_ipv4_address, uint16_t server_port,
                                                             TransportProtocol transport_protocol) {
    // get the machine name
    char hostname[MAX_MACHINENAME_LEN + 1];
    int error_code = gethostname(hostname, MAX_MACHINENAME_LEN);
    if (error_code < 0) {
        perror_msg("Failed to get the host name");
        return NULL;
    }
    hostname[MAX_MACHINENAME_LEN] = '\0';

    // get uid/gid of the calling process
    uid_t effective_uid = geteuid();
    gid_t effective_gid = getegid();

    // get the group IDs of the groups the process belongs to
    gid_t gids[MAX_N_GIDS];
    int n_gids = getgroups(MAX_N_GIDS, gids);
    if (n_gids < 0) {
        if (errno == EINVAL) {
            // this process belongs to more than MAX_N_GIDS groups (the list we gave it is not long enough)
            perror_msg("The process belongs to too many groups");
            return NULL;
        } else {
            perror_msg("Failed to get the supplementary group IDs");
            return NULL;
        }
    }

    Rpc__OpaqueAuth *credential = create_auth_sys_opaque_auth(hostname, effective_uid, effective_gid, n_gids, gids);

    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();

    return create_rpc_connection_context(server_ipv4_address, server_port, credential, verifier, transport_protocol);
}

/*
 * Deallocates all heap-allocated fields in the given RpcConnectionContext and the
 * RpcConnectionContext itself.
 *
 * Does nothing if the 'rpc_connection_context' is NULL.
 */
void free_rpc_connection_context(RpcConnectionContext *rpc_connection_context) {
    if (rpc_connection_context == NULL) {
        return;
    }

    free(rpc_connection_context->server_ipv4_addr);

    free_opaque_auth(rpc_connection_context->credential);
    free_opaque_auth(rpc_connection_context->verifier);

    switch (rpc_connection_context->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        // close the TCP client socket if it was correctly opened
        if (rpc_connection_context->transport_connection != NULL &&
            rpc_connection_context->transport_connection->tcp_rpc_client_socket_fd != NULL) {
            int tcp_rpc_client_socket_fd = *rpc_connection_context->transport_connection->tcp_rpc_client_socket_fd;
            if (tcp_rpc_client_socket_fd >= 0) {
                close(tcp_rpc_client_socket_fd);
            }

            free(rpc_connection_context->transport_connection->tcp_rpc_client_socket_fd);
        }

        free(rpc_connection_context->transport_connection);

        break;
    case TRANSPORT_PROTOCOL_QUIC:
        // close the QUIC connection if it was correctly set up
        if (rpc_connection_context->transport_connection != NULL &&
            rpc_connection_context->transport_connection->quic_client != NULL) {
            QuicClient *quic_client = rpc_connection_context->transport_connection->quic_client;

            if (quic_client != NULL) {
                // make the event loop thread execute the connection closing callback
                ev_async_send(quic_client->event_loop, &quic_client->connection_closing_async_watcher);

                pthread_mutex_lock(&quic_client->connection_closed_lock);
                while (!quic_client->connection_closed) {
                    pthread_cond_wait(&quic_client->connection_closed_condition_variable,
                                      &quic_client->connection_closed_lock);
                }
                pthread_mutex_unlock(&quic_client->connection_closed_lock);

                if (quic_client->successfully_created_event_loop_thread) {
                    pthread_join(quic_client->event_loop_thread, NULL);
                }

                if (quic_client->event_loop != NULL) {
                    ev_loop_destroy(quic_client->event_loop);
                }

                close(quic_client->socket_fd);

                if (quic_client->tls_config != NULL) {
                    quic_tls_config_free(quic_client->tls_config);
                }
                if (quic_client->quic_endpoint != NULL) {
                    quic_endpoint_free(quic_client->quic_endpoint);
                }
                if (quic_client->quic_config != NULL) {
                    quic_config_free(quic_client->quic_config);
                }

                free_stream_contexts_list(quic_client->stream_contexts, &quic_client->stream_contexts_list_lock,
                                          &quic_client->stream_contexts_condition_variable);

                pthread_mutex_destroy(&quic_client->stream_contexts_list_lock);
                pthread_cond_destroy(&quic_client->stream_contexts_condition_variable);

                free_streams_list(quic_client->auxiliary_streams_list_head, &quic_client->auxiliary_streams_list_lock);

                pthread_mutex_destroy(&quic_client->auxiliary_streams_list_lock);

                free(quic_client->main_stream);

                pthread_mutex_destroy(&quic_client->connection_established_lock);
                pthread_cond_destroy(&quic_client->connection_established_condition_variable);

                pthread_mutex_destroy(&quic_client->connection_closed_lock);
                pthread_cond_destroy(&quic_client->connection_closed_condition_variable);
            }

            free(quic_client);
        }

        free(rpc_connection_context->transport_connection);

        break;
    }

    free(rpc_connection_context);
}