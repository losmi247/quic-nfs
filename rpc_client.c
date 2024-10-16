#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <time.h> // for rand
#include <unistd.h> // read(), write(), close()

#include <protobuf-c/protobuf-c.h>
#include "serialization/rpc/rpc.pb-c.h"
#include "serialization/mount/mount.pb-c.h"

#include "rpc.h"

Google__Protobuf__Any *send_rpc_call_message(int rpc_client_socket_fd, uint32_t program_number, uint32_t program_version, uint32_t procedure_version, Google__Protobuf__Any parameters) {
    Rpc__CallBody call_body = RPC__CALL_BODY__INIT;
    call_body.rpcvers = 2;
    call_body.prog = program_number;
    call_body.vers = program_version;
    call_body.proc = procedure_version;
    call_body.params = &parameters;

    Rpc__RpcMsg rpc_msg = RPC__RPC_MSG__INIT;
    rpc_msg.xid = generate_rpc_xid();
    rpc_msg.mtype = RPC__MSG_TYPE__CALL;
    rpc_msg.body_case = RPC__RPC_MSG__BODY_CBODY; // this body_case enum is not actually sent over the network
    rpc_msg.cbody = &call_body;

    // serialize RpcMsg
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(&rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(rpc_msg_size);
    rpc__rpc_msg__pack(&rpc_msg, rpc_msg_buffer);

    // send serialized RpcMsg to server over network
    write(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);

    free(rpc_msg_buffer);
}

/*
void send_data(int client_socket_fd) {
    // Initialize a FhStatus message
    Nfs__Mount__FhStatus fh_status = NFS__MOUNT__FH_STATUS__INIT;
    fh_status.status = 0; // Success

    // Initialize a FHandle message
    Nfs__Mount__FHandle fh_handle = NFS__MOUNT__FHANDLE__INIT;
    const char *sample_handle = "sample_handle";
    fh_handle.handle.len = strlen(sample_handle); 
    fh_handle.handle.data = (uint8_t *) malloc(fh_handle.handle.len);
    memcpy(fh_handle.handle.data, sample_handle, fh_handle.handle.len); // Copy the sample handle to the data

    // Assign the FHandle to fh_status
    fh_status.directory = malloc(sizeof(Nfs__Mount__FHandle)); // Allocate memory for the FHandle pointer
    if (fh_status.directory == NULL) {
        fprintf(stderr, "Memory allocation for directory failed\n");
        free(fh_handle.handle.data); // Clean up previously allocated data
        exit(EXIT_FAILURE);
    }
    // Copy the FHandle data into the directory
    memcpy(fh_status.directory, &fh_handle, sizeof(Nfs__Mount__FHandle));

    // Serialize the message
    size_t size = nfs__mount__fh_status__get_packed_size(&fh_status);
    uint8_t *buffer = malloc(size);
    nfs__mount__fh_status__pack(&fh_status, buffer);

    // The 'buffer' now contains the serialized data - send it to server
    write(client_socket_fd, buffer, size);

    // Clean up
    free(fh_status.directory); // Free the allocated directory (FHandle)
    free(fh_handle.handle.data); // Free the allocated data for FHandle
    free(buffer);
}
*/

/*
* Takes in the RPC program to be called, program version, procedure number to be called, and the parameters for it.
* After calling the procedure, returns the server's response.
*/
Google__Protobuf__Any *invoke_rpc(const char *server_ipv4_addr, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters) {
    // for the rnd generator for xid
    srand(time(NULL));    

    // create TCP socket and verify
    int rpc_client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(rpc_client_socket_fd < 0) {
        fprintf(stderr, "Socket creation failed.");
        return NULL;
    }

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET;
    rpc_server_addr.sin_addr.s_addr = inet_addr(server_ipv4_addr); 
    rpc_server_addr.sin_port = htons(RPC_SERVER_PORT);

    // connect the rpc client socket to rpc server socket
    if(connect(rpc_client_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) {
        fprintf(stderr, "Connection with the server failed.");
        return NULL;
    }

    // need to implement time-outs + reconnections in case the server doesn't respond after some time
    Google__Protobuf__Any *procedure_results = send_rpc_call_message(rpc_client_socket_fd, program_number, program_version, procedure_number, parameters);

    close(rpc_client_socket_fd);

    return procedure_results;
}

int main() {
    // example of what the user (e.g. NFS client implementation) of this client will do:

    Nfs__Mount__DirPath dirpath = NFS__MOUNT__DIR_PATH__INIT;
    dirpath.path = "some/directory/path";
    // Serialize the DirPath message
    size_t dirpath_size = nfs__mount__dir_path__get_packed_size(&dirpath);
    uint8_t *dirpath_buffer = malloc(dirpath_size);
    nfs__mount__dir_path__pack(&dirpath, dirpath_buffer);

    // prepare the Any message to wrap DirPath
    Google__Protobuf__Any params = GOOGLE__PROTOBUF__ANY__INIT;
    params.type_url = "nfs.mount/DirPath"; // This should match your package and message name
    params.value.data = dirpath_buffer;
    params.value.len = dirpath_size;

    Google__Protobuf__Any *results = invoke_rpc("127.0.0.1",10005, 2, 1, params);
    // deserialize params here (not the job of RPC client)

    free(dirpath_buffer);

    //send_data(client_socket_fd);
    /*char *buff = (char *) malloc(200);
    snprintf(buff, 200, "hello I'm client");
    write(client_socket_fd, buff, 200);

    recv(client_socket_fd, buff, 200, 0);
    printf("Received from server: %s\n", buff);
    free(buff); */
}