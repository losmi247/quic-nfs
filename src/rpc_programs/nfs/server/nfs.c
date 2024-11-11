#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()
#include <signal.h> // for registering the SIGTERM handler

#include "src/common_rpc/server_common_rpc.h"

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"

#include "../nfs_common.h"

Rpc__AcceptedReply call_nfs(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

/*
* Functions from server_common_rpc.h that each RPC program's server must implement.
*/

Rpc__AcceptedReply forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    // since we're skipping the portmapper, any program number other than nfs's own is wrong
    if(program_number != NFS_RPC_PROGRAM_NUMBER) {
        // i don't need to set accepted_reply.default_case Empty ?
        fprintf(stderr, "Unknown program number");
        Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
        accepted_reply.stat = RPC__ACCEPT_STAT__PROG_UNAVAIL;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    return call_nfs(program_version, procedure_number, parameters);
}

/*
* Nfs RPC program implementation.
*/

int rpc_server_socket_fd;

/*
* Runs the NFSPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;

    // return an Any with empty buffer inside it
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "nfs/None";
    results->value.data = NULL;
    results->value.len = 0;

    accepted_reply.results = results;

    return accepted_reply;
}

/*
* Runs the NFSPROC_GETATTR procedure (1).
*/
Rpc__AcceptedReply serve_procedure_1_get_file_attributes(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/FHandle") != 0) {
        fprintf(stderr, "serve_procedure_1_get_file_attributes: Expected nfs/FHandle but received %s\n", parameters->type_url);
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // deserialize parameters
    Nfs__FHandle *fhandle = nfs__fhandle__unpack(NULL, parameters->value.len, parameters->value.data);
    if(fhandle == NULL) {
        fprintf(stderr, "serve_procedure_1_get_file_attributes: Failed to unpack FHandle\n");
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(fhandle->handle.data == NULL) {
        fprintf(stderr, "serve_procedure_1_get_file_attributes: FHandle->handle.data is null\n");

        nfs__fhandle__free_unpacked(fhandle, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    unsigned char *nfs_filehandle = fhandle->handle.data;

    

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;
    attr_stat.status = NFS__STAT__NFS_OK;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;

    Nfs__FAttr fattr = NFS__FATTR__INIT;
    fattr.type = NFS__FTYPE__NFDIR;
    fattr.mode = 0;
    fattr.nlink = 0;
    fattr.uid = fattr.gid = 0;
    fattr.size = fattr.blocksize = 0;
    fattr.rdev = fattr.blocks = fattr.fsid = fattr.fileid = 0;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT;
    atime.seconds = 0;
    atime.useconds = 0;
    Nfs__TimeVal mtime = NFS__TIME_VAL__INIT;
    mtime.seconds = 0;
    mtime.useconds = 0;
    Nfs__TimeVal ctime = NFS__TIME_VAL__INIT;
    ctime.seconds = 0;
    ctime.useconds = 0;

    fattr.atime = &atime;
    fattr.mtime = &mtime;
    fattr.ctime = &ctime;

    attr_stat.attributes = &fattr;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    // wrap procedure results into Any
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "nfs/AttrStat";
    results->value.data = attr_stat_buffer;
    results->value.len = attr_stat_size;

    // complete the AcceptedReply
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;
    accepted_reply.results = results;

    nfs__fhandle__free_unpacked(fhandle, NULL);

    return accepted_reply;
}

/*
* Calls the appropriate procedure in the Nfs RPC program based on the procedure number.
*/
Rpc__AcceptedReply call_nfs(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_version != 2) {
        Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

        // accepted_reply contains a pointer to mismatch_info, so mismatch_info has to be heap allocated - because it's going to be used outside of scope of this function
        Rpc__MismatchInfo *mismatch_info = malloc(sizeof(Rpc__MismatchInfo));
        rpc__mismatch_info__init(mismatch_info);
        mismatch_info->low = 2;
        mismatch_info->high = 2;

        accepted_reply.stat = RPC__ACCEPT_STAT__PROG_MISMATCH;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO;
        accepted_reply.mismatch_info = mismatch_info;

        return accepted_reply;
    }

    switch(procedure_number) {
        case 0:
            return serve_procedure_0_do_nothing(parameters);
        case 1:
            return serve_procedure_1_get_file_attributes(parameters);
        case 2:
        case 3:
        case 4:
        case 5:
        default:
    }

    // procedure not found
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__PROC_UNAVAIL;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
    return accepted_reply;
}

/*
* Signal handler for graceful shutdown
*/
void handle_signal(int signal) {
    if (signal == SIGTERM) {
        fprintf(stdout, "Received SIGTERM, shutting down gracefully...\n");
        
        close(rpc_server_socket_fd);

        exit(0);
    }
}

/*
* The main body of the Nfs server, which awaits RPCs.
*/
int main() {
    signal(SIGTERM, handle_signal);  // register signal handler

    rpc_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(rpc_server_socket_fd < 0) { 
        fprintf(stderr, "Socket creation failed\n");
        return 1;
    } 

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