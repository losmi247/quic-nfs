#include "src/repl/repl.h"

#include "src/fuse/handlers/handlers.h"

#include "src/repl/handlers/handlers.h"

#include <termios.h>

#include "src/common_rpc/rpc_connection_context.h"
#include "src/repl/filesystem_dag/filesystem_dag.h"
#include "src/parsing/parsing.h"


/*
* Nfs Client state.
*/

RpcConnectionContext *rpc_connection_context;

DAGNode *filesystem_dag_root;
DAGNode *cwd_node;

TransportProtocol chosen_transport_protocol;

int is_filesystem_mounted(void) {
    return rpc_connection_context != NULL && filesystem_dag_root != NULL && cwd_node != NULL;
}

/*
* Cleans up all Nfs client state before the REPL shuts down.
*/
void clean_up(void) {
    free_rpc_connection_context(rpc_connection_context);

    clean_up_dag(filesystem_dag_root);
}

// FUSE operation structure
static struct fuse_operations nfs_oper = {
    .getattr = nfs_getattr,
    .readdir = nfs_readdir,
    .read = nfs_read
};

int main(int argc, char *argv[]) {
    // parse command line arguments
    if(argc != 6) {
        fprintf(stderr, "Error: Incorrect usage. Correct usage: %s <IPv4 addr> <port number> --proto=<tcp or quic> <remote absolute path> <mount point> \n", argv[0]);
        return 1;
    }

    char *server_ipv4_address = argv[1];

    uint16_t port_number = parse_port_number(argv[2]);
    if(port_number == 0) {
        return 1;
    }

    const char *proto_flag = "--proto=";
    if(strncmp(argv[3], proto_flag, strlen(proto_flag)) == 0) {
        char *protocol = argv[3] + strlen(proto_flag);
        if(strcmp(protocol, "tcp") == 0) {
            chosen_transport_protocol = TRANSPORT_PROTOCOL_TCP;
        }
        else if(strcmp(protocol, "quic") == 0) {
            chosen_transport_protocol = TRANSPORT_PROTOCOL_QUIC;
        } 
        else {
            fprintf(stderr, "Error: Invalid transport protocol: %s\n", protocol);
            return 1;
        }
    }

    char *remote_absolute_path = argv[4];

    // initialize NFS client state
    rpc_connection_context = NULL;
    filesystem_dag_root = NULL;
    cwd_node = NULL;

    // mount the remote filesystem
    int error_code = handle_mount(server_ipv4_address, port_number, remote_absolute_path);
    if(error_code > 0) {
        printf("Failed to mount the remote filesystem\n");
        return 1;
    }

    char *args[] = {argv[0], argv[5], "-d"};
    return fuse_main(3, args, &nfs_oper, NULL);
}
