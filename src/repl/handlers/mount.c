#include "handlers.h"

#include "src/repl/validation/validation.h"

/*
* Given the IPv4 address and port number of a NFS server and an absolute path of a directory at the device
* where the server resides, attempts to mount this directory via NFS.
*
* Replaces any directories (from same or another NFS server) mounted so far with this new one, and sets the
* current working directory to the mounted directory.
*/
void handle_mount(const char *server_ip, uint16_t server_port, char *remote_absolute_path) {
    // clean up any previously mounted filesystem
    if(filesystem_dag_root != NULL) {
        free(server_ipv4_addr);
        clean_up_dag(filesystem_dag_root);
        cwd_node = filesystem_dag_root = NULL;

        server_ipv4_addr = NULL;
        server_port_number = 0;
    }

    printf("Mounting NFS share...\n");
    printf("Server IP: %s\n", server_ip);
    printf("Port: %d\n", server_port);
    printf("Remote Path: %s\n\n", remote_absolute_path);

    // mount the new NFS share
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = remote_absolute_path;

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(server_ip, server_port, dirpath, fhstatus);
    if(status != 0) {
        free(fhstatus);

        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        return;
    }

    if(validate_mount_fh_status(fhstatus) > 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        printf("Error: Invalid NFS procedure result received from the server\n");

        return;
    }

    if(fhstatus->mnt_status->stat != MOUNT__STAT__MNT_OK) {
        char *string_status = mount_stat_to_string(fhstatus->mnt_status->stat);
        printf("Error: Failed to mount the NFS share with status %s\n", string_status);
        free(string_status);

        mount__fh_status__free_unpacked(fhstatus, NULL);

        return;
    }

    // initialize the filesystem DAG
    NfsFh__NfsFileHandle *nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    *nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);

    Nfs__FHandle *fhandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(fhandle);
    fhandle->nfs_filehandle = nfs_filehandle_copy;

    filesystem_dag_root = create_dag_node(strdup(remote_absolute_path), NFS__FTYPE__NFDIR, fhandle, 1);
    if(filesystem_dag_root == NULL) {
        free(nfs_filehandle_copy);
        free(fhandle);

        printf("Error: Failed to create a new filesystem DAG node\n");

        return;
    }

    cwd_node = filesystem_dag_root;

    // set the server address and port
    server_ipv4_addr = strdup(server_ip);
    server_port_number = server_port;
}