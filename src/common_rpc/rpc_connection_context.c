#include "rpc_connection_context.h"

#include <unistd.h>
#include <arpa/inet.h>

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
    if(rpc_connection_context == NULL) {
        return 1;
    }

    if(rpc_connection_context->server_ipv4_addr == NULL) {
        return 2;
    }

    rpc_connection_context->tcp_rpc_client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(rpc_connection_context->tcp_rpc_client_socket_fd < 0) {
        perror_msg("Socket creation failed");
        return 3;
    }

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET;
    rpc_server_addr.sin_addr.s_addr = inet_addr(rpc_connection_context->server_ipv4_addr); 
    rpc_server_addr.sin_port = htons(rpc_connection_context->server_port);

    // connect the rpc client socket to rpc server socket
    if(connect(rpc_connection_context->tcp_rpc_client_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) {
        perror_msg("Connection to the server failed");

        close(rpc_connection_context->tcp_rpc_client_socket_fd);

        return 4;
    }

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
RpcConnectionContext *create_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port, Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, TransportProtocol transport_protocol) {
    RpcConnectionContext *rpc_connection_context = malloc(sizeof(RpcConnectionContext));

    rpc_connection_context->server_ipv4_addr = strdup(server_ipv4_addres);
    rpc_connection_context->server_port = server_port;

    rpc_connection_context->credential = credential;
    rpc_connection_context->verifier = verifier;

    rpc_connection_context->transport_protocol = transport_protocol;
    switch(transport_protocol) {
        case TRANSPORT_PROTOCOL_TCP:
            int error_code = connect_to_tcp_server(rpc_connection_context);
            if(error_code > 0) {
                printf("Failed to connect to the server with error code %d\n", error_code);

                free(rpc_connection_context->server_ipv4_addr);
                free(rpc_connection_context);

                return NULL;
            }
            break;
        case TRANSPORT_PROTOCOL_QUIC:
            // establish a QUIC connection
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
RpcConnectionContext *create_auth_none_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port, TransportProtocol transport_protocol) {
    Rpc__OpaqueAuth *credential = create_auth_none_opaque_auth();

    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();

    return create_rpc_connection_context(server_ipv4_addres, server_port, credential, verifier, transport_protocol);
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
RpcConnectionContext *create_auth_sys_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port, TransportProtocol transport_protocol) {
    // get the machine name
    char hostname[MAX_MACHINENAME_LEN+1];
    int error_code = gethostname(hostname, MAX_MACHINENAME_LEN);
    if(error_code < 0) {
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
    if(n_gids < 0) {
        if(errno == EINVAL) {
            // this process belongs to more than MAX_N_GIDS groups (the list we gave it is not long enough)
            perror_msg("The process belongs to too many groups");
            return NULL;
        }
        else {
            perror_msg("Failed to get the supplementary group IDs");
            return NULL;
        }
    }

    Rpc__OpaqueAuth *credential = create_auth_sys_opaque_auth(hostname, effective_uid, effective_gid, n_gids, gids);    

    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();

    return create_rpc_connection_context(server_ipv4_addres, server_port, credential, verifier, transport_protocol);
}

/*
* Deallocates all heap-allocated fields in the given RpcConnectionContext and the
* RpcConnectionContext itself.
*
* Does nothing if the 'rpc_connection_context' is NULL.
*/
void free_rpc_connection_context(RpcConnectionContext *rpc_connection_context) {
    if(rpc_connection_context == NULL) {
        return;
    }

    free(rpc_connection_context->server_ipv4_addr);
    
    free_opaque_auth(rpc_connection_context->credential);
    free_opaque_auth(rpc_connection_context->verifier);

    if(rpc_connection_context->transport_protocol == TRANSPORT_PROTOCOL_TCP) {
        // close the TCP client socket if it was correctly opened
        if(rpc_connection_context->tcp_rpc_client_socket_fd >= 0) {
            close(rpc_connection_context->tcp_rpc_client_socket_fd);
        }
    }

    free(rpc_connection_context);
}