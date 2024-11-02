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

/*
* Mount RPC program implementation.
*/

/*
* MountList management.
*/
Mount__MountList *mount_list;

void add_mount_entry(Mount__MountList *new_mount_entry) {
    new_mount_entry->nextentry = mount_list;
    mount_list = new_mount_entry;
}

void free_mount_list_entry(Mount__MountList *mount_list_entry) {
    free(mount_list_entry->hostname->name);
    free(mount_list_entry->hostname);

    mount__dir_path__free_unpacked(mount_list_entry->directory, NULL);

    free(mount_list_entry);
}

void cleanup_mount_list(Mount__MountList *list_head) {
    if(list_head == NULL) {
        return;
    }

    cleanup_mount_list(list_head->nextentry);
    free_mount_list_entry(list_head);
}

/*
* Exported files management.
*/
int is_directory_exported(const char *absolute_path) {
    if(absolute_path == NULL) {
        return 0;
    }

    FILE *file = fopen("./exports", "r");
    if(file == NULL) {
        fprintf(stderr, "Mount server failed to open /etc/exports\n");
        return 0;
    }

    int is_exported = 0;
    char line[1024];
    // read file line by line
    while(fgets(line, sizeof(line), file) != NULL) {
        // remove trailing newline if present
        line[strcspn(line, "\n")] = '\0';

        // check if the line starts with absolute_path
        if (strncmp(line, absolute_path, strlen(absolute_path)) == 0) {
            const char *options_start = line + strlen(absolute_path);
            // check for whitespace after the path
            if((*options_start == ' ' || *options_start == '\t') && strstr(options_start, "*(rw)") != NULL) {
                is_exported = 1;
                break;
            }
        }
    }

    fclose(file);

    return is_exported;
}

/*
* Creates a filehandle for the given absolute path of a directory being mounted.
*
* For now filehandles are literally the absolute paths themselves.
*/
uint8_t *get_filehandle(char *directory_absolute_path) {
    if(directory_absolute_path == NULL) {
        return (uint8_t *)"";
    }

    return (uint8_t *)directory_absolute_path;
}

/*
* Runs the MOUNTPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;

    // return an Any with empty buffer inside it
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "mount/None";
    results->value.data = NULL;
    results->value.len = 0;

    accepted_reply.results = results;

    return accepted_reply;
}

/*
* Runs the MOUNTPROC_MNT procedure (1).
*/
Rpc__AcceptedReply serve_procedure_1_add_mount_entry(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "mount/DirPath") != 0) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: Expected mount/DirPath but received %s\n", parameters->type_url);
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // deserialize parameters
    // this DirPath is freed at server shutdown as part of mountlist cleanup - it needs to be persisted here at server!
    Mount__DirPath *dirpath = mount__dir_path__unpack(NULL, parameters->value.len, parameters->value.data);
    if(dirpath == NULL) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: Failed to unpack DirPath\n");
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(dirpath->path == NULL) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: DirPath->path is null\n");
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // check the server has exported this directory for NFS
    if(!is_directory_exported(dirpath->path)) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: directory %s not exported for NFS\n", dirpath->path);
        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // create a new mount entry
    Mount__MountList *new_mount_entry = malloc(sizeof(Mount__MountList));
    mount__mount_list__init(new_mount_entry);
    new_mount_entry->hostname = malloc(sizeof(Mount__Name));
    mount__name__init(new_mount_entry->hostname);
    new_mount_entry->hostname->name = strdup("client-hostname"); // replace with actual hostname when available
    new_mount_entry->directory = dirpath;

    add_mount_entry(new_mount_entry);

    // build the procedure results
    Mount__FhStatus fh_status = MOUNT__FH_STATUS__INIT;
    fh_status.status = 0;
    fh_status.fhstatus_body_case = MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY;

    // get a file handle for this directory
    uint8_t *filehandle = get_filehandle(dirpath->path);
    Mount__FHandle fhandle = MOUNT__FHANDLE__INIT;
    fhandle.handle.data = filehandle;
    fhandle.handle.len = strlen(filehandle);

    fh_status.directory = &fhandle;

    // serialize the procedure results
    size_t fh_status_size = mount__fh_status__get_packed_size(&fh_status);
    uint8_t *fh_status_buffer = malloc(fh_status_size);
    mount__fh_status__pack(&fh_status, fh_status_buffer);

    // wrap procedure results into Any
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "mount/FhStatus";
    results->value.data = fh_status_buffer;
    results->value.len = fh_status_size;

    // complete the AcceptedReply
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;
    accepted_reply.results = results;

    // free up memory - it's okay for dirpath to be deallocated here - serialization made a copy of all values and puts it all into buffer
    mount__dir_path__free_unpacked(dirpath, NULL);

    return accepted_reply;
}

/*
* Calls the appropriate procedure in the Mount RPC program based on the procedure number.
*/
Rpc__AcceptedReply call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_version != 2) {
        Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

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
        case 0:
            return serve_procedure_0_do_nothing(parameters);
        case 1:
            return serve_procedure_1_add_mount_entry(parameters);
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

    // initialise mount list to be empty
    mount_list = NULL;
  
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

    cleanup_mount_list(mount_list);
} 