#include "caller.h"

Google__Protobuf__Any *call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    switch(procedure_number) {
        case 1: // call Mount program implementation from a different file
        case 2:
        default:
    }

    /*
    uint8_t *dirpath_buffer = parameters->value.data;
    size_t dirpath_size = parameters->value.len;
    Nfs__Mount__DirPath *dirpath = nfs__mount__dir_path__unpack(NULL, dirpath_size, dirpath_buffer);
    printf("RPC parameters: \n");
    printf("    dirpath: %s\n", dirpath_buffer);

    // cleanup
    nfs__mount__dir_path__free_unpacked(dirpath, NULL);
    */

    // call the procedure, pack its reults into Any, and return them
    return parameters;
}