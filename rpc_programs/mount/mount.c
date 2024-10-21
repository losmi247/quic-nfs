#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()

#include "common_rpc/server_common_rpc.h"

#include <protobuf-c/protobuf-c.h>
#include "serialization/rpc/rpc.pb-c.h"
#include "serialization/mount/mount.pb-c.h"

#include "mount.h"

Rpc__AcceptedReply call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

/*
* Functions from common_rpc.h that each RPC program's server must implement.
*/

Rpc__AcceptedReply forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    printf("Program number: %d\n", program_number);
    printf("Program version: %d\n", program_version);
    printf("Procedure number: %d\n", procedure_number);

    // since we're skipping the portmapper, any program number other than mount's own is wrong
    if(program_number != MOUNT_RPC_PROGRAM_NUMBER) {
        // i don't need to set accepted_reply.default_case Empty ?
        fprintf(stderr, "Unknown program number");
        Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
        accepted_reply.stat = RPC__ACCEPT_STAT__PROG_UNAVAIL;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    return call_mount(program_version, procedure_number, parameters);
}

void clean_up_accepted_reply(Rpc__AcceptedReply accepted_reply) {
    if(accepted_reply.stat == RPC__ACCEPT_STAT__PROG_MISMATCH) {
        free(accepted_reply.mismatch_info);
        return;
    }

    if(accepted_reply.stat == RPC__ACCEPT_STAT__SUCCESS) {
        // clean up procedure results
    }
}

/*
* Mount RPC program implementation.
*/

/*
* Calls the appropriate procedure in the Mount RPC program based on the procedure number.
*/
Rpc__AcceptedReply call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

    if(program_version != 2) {
        // accepted_reply contains a pointer to mismatch_info, so mismatch_info has to be heap allocated - because it's going to be used outside of scope of this function
        Rpc__MismatchInfo *mismatch_info = malloc(sizeof(Rpc__MismatchInfo));
        Rpc__MismatchInfo initialized_mismatch_info = RPC__MISMATCH_INFO__INIT;
        memcpy(mismatch_info, &initialized_mismatch_info, sizeof(Rpc__MismatchInfo));
        mismatch_info->low = 2;
        mismatch_info->high = 2;

        accepted_reply.stat = RPC__ACCEPT_STAT__PROG_MISMATCH;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO;
        accepted_reply.mismatch_info = mismatch_info;

        return accepted_reply;
    }

    switch(procedure_number) {
        // for each procedure: 1. deserialize arguments, return GARBAGE_ARGS AcceptStat if unsuccessful
        //                     2. on success, call the procedure with those arguments, return accepted_result with SUCCESS and procedure results
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        default:
    }

    // procedure not found
    accepted_reply.stat = RPC__ACCEPT_STAT__PROC_UNAVAIL;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
    return accepted_reply;

    /*
    // Deserialize the buffer back into a FhStatus message
    Nfs__Mount__FhStatus *fh_status = nfs__mount__fh_status__unpack(NULL, bytes_received, buffer);
    if (fh_status == NULL) {
        fprintf(stderr, "Error unpacking received message\n");
        exit(EXIT_FAILURE);
    }
    // Access the deserialized fields
    printf("Status: %u\n", fh_status->status);
    if (fh_status->status == 0 && fh_status->directory != NULL) {
        printf("Directory Handle Length: %lu\n", fh_status->directory->handle.len);
        printf("Directory Handle Data: %.*s\n", (int)fh_status->directory->handle.len, fh_status->directory->handle.data);
    }
    else {
        printf("No valid directory handle received (status: %u)\n", fh_status->status);
    }

    // Free the allocated message
    nfs__mount__fh_status__free_unpacked(fh_status, NULL);
    */

    /*
    char *response = (char *) malloc(BUFFER_SIZE * sizeof(char));
    snprintf(response, BUFFER_SIZE, "hello");
    send(client_socket_fd, response, BUFFER_SIZE, 0);
    */
}

/*
* The main body of the Mount server, which awaits RPCs.
*/
int main() {
    int rpc_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(rpc_server_socket_fd < 0) { 
        perror("Socket creation failed");
        return 1;
    } 

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET; 
    rpc_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rpc_server_addr.sin_port = htons(MOUNT_RPC_SERVER_PORT);
  
    // bind socket to the rpc server address
    if(bind(rpc_server_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) { 
        perror("Socket bind failed");
        close(rpc_server_socket_fd);
        return 1;
    }
  
    // listen for connections on the port
    if(listen(rpc_server_socket_fd, 10) < 0) {
        perror("Listen failed");
        close(rpc_server_socket_fd);
        return 1;
    }
  
    while(1) {
        struct sockaddr_in rpc_client_addr;
        socklen_t rpc_client_addr_len = sizeof(rpc_client_addr);

        int rpc_client_socket_fd = accept(rpc_server_socket_fd, (struct sockaddr *) &rpc_client_addr, &rpc_client_addr_len);
        if(rpc_client_socket_fd < 0) { 
            perror("Server failed to accept connection"); 
            continue;
        }
    
        handle_client(rpc_client_socket_fd);

        close(rpc_client_socket_fd);
    }
     
    close(rpc_server_socket_fd);
} 