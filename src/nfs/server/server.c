#include "server.h"

/*
* Define Mount+Nfs server state
*/

TransportProtocol transport_protocol;

Mount__MountList *mount_list;
InodeCache inode_cache;

ReadDirSessionsList *readdir_sessions_list;
pthread_t periodic_cleanup_thread;

/*
* Functions from server_common_rpc.h that each RPC program's server must implement.
*/

/*
* Forwards the RPC call to the specific RPC program, and returns the AcceptedReply.
*
* Also forwards a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported authentication
* flavor) before being passed here.
*
* The user of this function takes the responsibility to deallocate the returned AcceptedReply
* and any heap-allocated fields in it (this is done by the 'clean_up_accepted_reply' function after the RPC is sent).
*/
Rpc__AcceptedReply *forward_rpc_call_to_program(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_number == MOUNT_RPC_PROGRAM_NUMBER) {
        return call_mount(credential, verifier, program_version, procedure_number, parameters);
    }
    if(program_number == NFS_RPC_PROGRAM_NUMBER) {
        return call_nfs(credential, verifier, program_version, procedure_number, parameters);
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
    if(signal == SIGTERM) {
        fprintf(stdout, "Received SIGTERM, shutting down gracefully...\n");

        switch(transport_protocol) {
            case TRANSPORT_PROTOCOL_TCP:
                clean_up_tcp_server_state();
                break;
            case TRANSPORT_PROTOCOL_QUIC:
                clean_up_quic_server_state();
                break;
            default:
        }

        clean_up_inode_cache(inode_cache);
        clean_up_mount_list(mount_list);

        // wait for the periodic cleanup thread to terminate
        pthread_cancel(periodic_cleanup_thread);
        pthread_join(periodic_cleanup_thread, NULL);

        clean_up_readdir_sessions_list(readdir_sessions_list);    //  this function is not atomic, but the cleanup thread has been terminated now, so it's fine

        fprintf(stdout, "Server shutdown successfull\n");
        fflush(stdout);

        exit(0);
    }
}

/*
* The thread that periodically iterates through all ReadDir sessions
* and discards the expired ones.
*/
void *readdir_periodic_cleanup_thread(void *arg) {
    while(1) {
        sleep(PERIODIC_CLEANUP_SLEEP_TIME);
        clean_up_expired_readdir_sessions(&readdir_sessions_list);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // parse command line arguments
    if(argc != 3) {
        fprintf(stderr, "Error: Incorrect usage. Correct usage: %s (<port number> or --test) (--proto=tcp or quic)\n", argv[0]);
        return 1;
    }

    uint16_t port_number = 0;
    if(strncmp(argv[1], "--", 2) == 0) {    // the first argument is a flag
        if(strcmp(argv[1], "--test") == 0) {
            port_number = NFS_AND_MOUNT_TEST_RPC_SERVER_PORT;   // when running tests, run at this port
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

    const char *proto_flag = "--proto=";
    if(strncmp(argv[2], proto_flag, strlen(proto_flag)) == 0) {
        char *protocol = argv[2] + strlen(proto_flag);
        if(strcmp(protocol, "tcp") == 0) {
            transport_protocol = TRANSPORT_PROTOCOL_TCP;
        }
        else if(strcmp(protocol, "quic") == 0) {
            transport_protocol = TRANSPORT_PROTOCOL_QUIC;
        } 
        else {
            fprintf(stderr, "Error: Invalid transport protocol: %s\n", protocol);
            return 1;
        }
    }

    // register SIGTERM handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    // ensure that system calls are restarted if interrupted by this signal
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("Failed to register SIGTERM handler");
        return 1;
    }

    // initialize Nfs and Mount server state
    mount_list = NULL;
    inode_cache = NULL;
    readdir_sessions_list = NULL;

    // start the periodic cleanup thread
    if (pthread_create(&periodic_cleanup_thread, NULL, readdir_periodic_cleanup_thread, NULL) != 0) {
        perror("Failed to create cleanup thread");
        return 1;
    }

    // run the Nfs+Mount server
    switch(transport_protocol) {
        case TRANSPORT_PROTOCOL_TCP:
            return run_server_tcp(port_number);
        case TRANSPORT_PROTOCOL_QUIC:
            return run_server_quic(port_number);
        default:
            return run_server_tcp(port_number);
    }
}