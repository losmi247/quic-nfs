#include "server.h"

/*
* Define Mount+Nfs server state
*/
int rpc_server_socket_fd;

Mount__MountList *mount_list;
InodeCache inode_cache;

/*
* Functions from server_common_rpc.h that each RPC program's server must implement.
*/

/*
* Forwards the RPC call to the specific RPC program, and returns the AcceptedReply.
*/
Rpc__AcceptedReply forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_number == MOUNT_RPC_PROGRAM_NUMBER) {
        return call_mount(program_version, procedure_number, parameters);
    }
    if(program_number == NFS_RPC_PROGRAM_NUMBER) {
        return call_nfs(program_version, procedure_number, parameters);
    }

    fprintf(stderr, "Unknown program number");
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__PROG_UNAVAIL;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    accepted_reply.default_case = empty;

    return accepted_reply;
}

/*
* Functions used in implementation of server body
*/

/*
* Signal handler for graceful shutdown.
*/
void handle_signal(int signal) {
    if (signal == SIGTERM) {
        fprintf(stdout, "Received SIGTERM, shutting down gracefully...\n");
        
        close(rpc_server_socket_fd);

        clean_up_inode_cache(inode_cache);
        clean_up_mount_list(mount_list);

        fprintf(stdout, "Server shutdown successfull\n");

        exit(0);
    }
}

/*
* The main body of the Nfs+Mount server, which awaits RPCs.
*/
int main() {
    signal(SIGTERM, handle_signal);  // register signal handler

    rpc_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(rpc_server_socket_fd < 0) { 
        fprintf(stderr, "Socket creation failed\n");
        return 1;
    }

    // initialize Nfs and Mount server state
    mount_list = NULL;
    inode_cache = NULL;

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET; 
    rpc_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rpc_server_addr.sin_port = htons(NFS_RPC_SERVER_PORT);
  
    // bind socket to the rpc server address
    if(bind(rpc_server_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) { 
        fprintf(stderr, "Socket bind failed\n");
        close(rpc_server_socket_fd);
        return 1;
    }
  
    // listen for connections on the port
    if(listen(rpc_server_socket_fd, 10) < 0) {
        fprintf(stderr, "Listen failed\n");
        close(rpc_server_socket_fd);
        return 1;
    }

    fprintf(stdout, "Server listening on port %d\n", NFS_RPC_SERVER_PORT);
  
    while(1) {
        struct sockaddr_in rpc_client_addr;
        socklen_t rpc_client_addr_len = sizeof(rpc_client_addr);

        int rpc_client_socket_fd = accept(rpc_server_socket_fd, (struct sockaddr *) &rpc_client_addr, &rpc_client_addr_len);
        if(rpc_client_socket_fd < 0) { 
            fprintf(stderr, "Server failed to accept connection\n"); 
            continue;
        }
    
        handle_client(rpc_client_socket_fd);

        close(rpc_client_socket_fd);
    }
     
    close(rpc_server_socket_fd);
}