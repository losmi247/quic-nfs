#include "handlers.h"

/*
 * Given the IPv4 address and port number of a NFS server and an absolute path of a directory at the device
 * where the server resides, attempts to mount this directory via NFS.
 *
 * Replaces any directories (from same or another NFS server) mounted so far with this new one, and sets the
 * current working directory to the mounted directory.
 *
 * Returns 0 on success and > 0 on failure.
 */
int handle_mount(char *server_ip, uint16_t server_port, char *remote_absolute_path) {
    // clean up any previously mounted filesystem
    free_rpc_connection_context(rpc_connection_context);
    rpc_connection_context = NULL;

    clean_up_dag(filesystem_dag_root);
    cwd_node = filesystem_dag_root = NULL;

    printf("Mounting NFS share...\n");
    printf("Server IP: %s\n", server_ip);
    printf("Port: %d\n", server_port);
    printf("Remote Path: %s\n\n", remote_absolute_path);

    // create a new RPC connection context
    RpcConnectionContext *new_rpc_connection_context =
        create_auth_sys_rpc_connection_context(server_ip, server_port, chosen_transport_protocol);
    if (new_rpc_connection_context == NULL) {
        printf("Error: Failed to create AUTH_SYS Rpc connection context\n");
        return 1;
    }

    // mount the new NFS share
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = remote_absolute_path;

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(new_rpc_connection_context, dirpath, fhstatus);
    if (status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free_rpc_connection_context(new_rpc_connection_context);
        free(fhstatus);

        return 1;
    }

    if (validate_mount_fh_status(fhstatus) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        free_rpc_connection_context(new_rpc_connection_context);
        mount__fh_status__free_unpacked(fhstatus, NULL);

        return 1;
    }

    if (fhstatus->mnt_status->stat == MOUNT__STAT__MNTERR_ACCES) {
        printf("mount: Permission denied: %s\n", remote_absolute_path);

        free_rpc_connection_context(new_rpc_connection_context);
        mount__fh_status__free_unpacked(fhstatus, NULL);

        return 1;
    } else if (fhstatus->mnt_status->stat != MOUNT__STAT__MNT_OK) {
        char *string_status = mount_stat_to_string(fhstatus->mnt_status->stat);
        printf("Error: Failed to mount the NFS share with status %s\n", string_status);
        free(string_status);

        free_rpc_connection_context(new_rpc_connection_context);
        mount__fh_status__free_unpacked(fhstatus, NULL);

        return 1;
    }

    // initialize the filesystem DAG
    NfsFh__NfsFileHandle *nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    *nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);

    Nfs__FHandle *fhandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(fhandle);
    fhandle->nfs_filehandle = nfs_filehandle_copy;

    filesystem_dag_root = create_dag_node(remote_absolute_path, NFS__FTYPE__NFDIR, fhandle, 1);
    if (filesystem_dag_root == NULL) {
        printf("Error: Failed to create a new filesystem DAG node\n");

        free_rpc_connection_context(new_rpc_connection_context);
        free(nfs_filehandle_copy);
        free(fhandle);

        return 1;
    }

    cwd_node = filesystem_dag_root;

    // set the new Rpc connection context
    rpc_connection_context = new_rpc_connection_context;

    return 0;
}