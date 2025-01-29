#include "nfs_fuse.h"

#include "src/fuse/handlers/handlers.h"

#include "src/parsing/parsing.h"
#include "src/filehandle_management/filehandle_management.h"

#include "src/nfs/clients/nfs_client.h"
#include "src/nfs/clients/mount_client.h"

static struct fuse_operations nfs_oper = {
    .getattr = nfs_getattr,

    .readdir = nfs_readdir,
    .read = nfs_read,
    .write = nfs_write,
    .readlink = nfs_readlink,

    .mknod = nfs_mknod,
    .mkdir = nfs_mkdir,
    .symlink = nfs_symlink,

    .utimens = nfs_utimens,
    .truncate = nfs_truncate,

    .open = nfs_open,

    .rmdir = nfs_rmdir,
    .unlink = nfs_unlink
};

/*
* Nfs Client state.
*/

RpcConnectionContext *rpc_connection_context;

Nfs__FHandle *filesystem_root_fhandle;

TransportProtocol chosen_transport_protocol;

/*
* Cleans up all Nfs client state before the client shuts down.
*/
void clean_up(void) {
    free_rpc_connection_context(rpc_connection_context);

    free(filesystem_root_fhandle->nfs_filehandle);
    free(filesystem_root_fhandle);
}

/*
* Sends a MOUNT RPC to the remote server to get the root filehandle of the NFS share at
* the given remote absolute path on the server.
*
* Returns 0 if the remote filesystem is successfully mounted, and > 0 on failure.
*/
int mount_remote_filesystem(char *server_ipv4_address, uint16_t server_port, char *remote_absolute_path) {
    printf("Mounting remote NFS share...\n");
    printf("Server IP: %s\n", server_ipv4_address);
    printf("Port: %d\n", server_port);
    printf("Remote Path: %s\n\n", remote_absolute_path);

    // create a new RPC connection context
    rpc_connection_context = create_auth_sys_rpc_connection_context(server_ipv4_address, server_port, chosen_transport_protocol);
    if(rpc_connection_context == NULL) {
        printf("Error: Failed to create AUTH_SYS Rpc connection context\n");
        return 1;
    }

    // mount the NFS share
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = remote_absolute_path;

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(rpc_connection_context, dirpath, fhstatus);
    if(status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free_rpc_connection_context(rpc_connection_context);
        free(fhstatus);

        return 1;
    }

    if(validate_mount_fh_status(fhstatus) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        free_rpc_connection_context(rpc_connection_context);
        mount__fh_status__free_unpacked(fhstatus, NULL);

        return 1;
    }

    if(fhstatus->mnt_status->stat == MOUNT__STAT__MNTERR_ACCES) {
        printf("mount: Permission denied: %s\n", remote_absolute_path);
        
        free_rpc_connection_context(rpc_connection_context);
        mount__fh_status__free_unpacked(fhstatus, NULL);

        return 1;
    }
    else if(fhstatus->mnt_status->stat != MOUNT__STAT__MNT_OK) {
        char *string_status = mount_stat_to_string(fhstatus->mnt_status->stat);
        printf("Error: Failed to mount the NFS share with status %s\n", string_status);
        free(string_status);

        free_rpc_connection_context(rpc_connection_context);
        mount__fh_status__free_unpacked(fhstatus, NULL);

        return 1;
    }

    // initialize the filesystem DAG
    NfsFh__NfsFileHandle *nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    if(nfs_filehandle_copy == NULL) {
        free_rpc_connection_context(rpc_connection_context);
        mount__fh_status__free_unpacked(fhstatus, NULL);

        return 1;
    }
    *nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);

    filesystem_root_fhandle = malloc(sizeof(Nfs__FHandle));
    if(filesystem_root_fhandle == NULL) {
        free_rpc_connection_context(rpc_connection_context);
        free(nfs_filehandle_copy);

        return 1;
    }
    nfs__fhandle__init(filesystem_root_fhandle);
    filesystem_root_fhandle->nfs_filehandle = nfs_filehandle_copy;

    return 0;
}

int main(int argc, char *argv[]) {
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
    filesystem_root_fhandle = NULL;

    // mount the remote filesystem
    int error_code = mount_remote_filesystem(server_ipv4_address, port_number, remote_absolute_path);
    if(error_code > 0) {
        printf("Failed to mount the remote filesystem\n");
        return 1;
    }

    char *args[] = {argv[0], argv[5], "-d"};
    return fuse_main(3, args, &nfs_oper, NULL);
}
