#include "server.h"

#include "tests/test_common.h"  // for NFS_AND_MOUNT_TEST_RPC_SERVER_PORT
#include "src/parsing/parsing.h"       // for parsing the port number from command line args

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
*
* The user of this function takes the responsibility to deallocate the returned AcceptedReply
* and any heap-allocated fields in it (this is done by the 'clean_up_accepted_reply' function after the RPC is sent).
*/
Rpc__AcceptedReply *forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_number == MOUNT_RPC_PROGRAM_NUMBER) {
        return call_mount(program_version, procedure_number, parameters);
    }
    if(program_number == NFS_RPC_PROGRAM_NUMBER) {
        return call_nfs(program_version, procedure_number, parameters);
    }

    fprintf(stderr, "Unknown program number");
    return create_default_case_accepted_reply(RPC__ACCEPT_STAT__PROG_UNAVAIL);
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
        fflush(stdout);

        exit(0);
    }
}

/*
* The main body of the Nfs+Mount server, which awaits RPCs.
*/
int main(int argc, char *argv[]) {
    // parse command line arguments
    if(argc != 2) {
        fprintf(stderr, "Error: Incorrect usage. Correct usage: %s (<port number> or --test)\n", argv[0]);
        return 1;
    }

    uint16_t port_number = 0;
    if(strncmp(argv[1], "--", 2) == 0) {    // the first argument is a flag
        if(strcmp(argv[1], "--test") == 0) {
            port_number = NFS_AND_MOUNT_TEST_RPC_SERVER_PORT;   // when testing, run at this port
        }
        else {
            fprintf(stderr, "Error: Invalid flag. Did you mean '--test'?\n");
            return 1;
        }
    }
    else {
        port_number = parse_port_number(argv[1]);
        if(port_number == 0) {
            return 1;
        }
    }

    // register the signal handler
    signal(SIGTERM, handle_signal);  

    // create the server socket
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
    rpc_server_addr.sin_port = htons(port_number);
  
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

    fprintf(stdout, "Server listening on port %d...\n", port_number);
  
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