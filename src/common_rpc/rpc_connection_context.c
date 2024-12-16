#include "rpc_connection_context.h"

/*
* Creates a RpcConnectionContext with the given server IP address and server port,
* and the given credential and verifier.
*
* The user of this function takes the responsiblity to free the allocated server_ipv4_addr string
* and the RpcConnectionContext itself.
*/
RpcConnectionContext *create_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port, Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier) {
    RpcConnectionContext *rpc_connection_context = malloc(sizeof(RpcConnectionContext));

    rpc_connection_context->server_ipv4_addr = strdup(server_ipv4_addres);
    rpc_connection_context->server_port = server_port;

    rpc_connection_context->credential = credential;
    rpc_connection_context->verifier = verifier;

    return rpc_connection_context;
}

/*
* Creates a RpcConnectionContext with the given server IP address and server port,
* the credential and verifier both having flavor AUTH_NONE.
*
* The user of this function takes the responsiblity to deallocate the received RpcConnectionContext using
* free_rpc_connection_context() function.
*/
RpcConnectionContext *create_auth_none_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port) {
    Rpc__OpaqueAuth *credential = create_auth_none_opaque_auth();

    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();

    return create_rpc_connection_context(server_ipv4_addres, server_port, credential, verifier);
}

/*
* Creates a RpcConnectionContext with the given server IP address and server port,
* the credential having flavor AUTH_SYS with machine name being the name of the machine on which the process
* that calls this function runs, uid/gid being the effective uid/gid of the calling process, and the list of
* groups gids being the list of groups that the calling process belongs too, and the verifier having flavour
* AUTH_NONE.
*
* Returns the constructed RpcConnectionContext on success, and NULL on failure.
*
* The user of this function takes the responsiblity to deallocete the received RpcConnectionContext using
* free_rpc_connection_context() function.
*/
RpcConnectionContext *create_auth_sys_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port) {
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

    return create_rpc_connection_context(server_ipv4_addres, server_port, credential, verifier);
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

    free(rpc_connection_context);
}