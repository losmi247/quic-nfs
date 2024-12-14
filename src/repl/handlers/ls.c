#include "handlers.h"

#include "src/repl/validation/validation.h"

/*
* If there is a directory that is currently mounted, prints out all directory entries in the current
* working directory.
*/
void handle_ls() {
    if(filesystem_dag_root == NULL) {
        printf("Error: No remote file system is currently mounted\n");
        return;
    }

    Nfs__NfsCookie nfs_cookie = NFS__NFS_COOKIE__INIT;
    nfs_cookie.value = 0;

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = cwd_node->fhandle;
    readdirargs.cookie = &nfs_cookie;
    readdirargs.count = 1000;   // try to read all entries

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(server_ipv4_addr, server_port_number, readdirargs, readdirres);
    if(status != 0) {
        free(readdirres);

        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        return;
    }

    if(validate_nfs_read_dir_res(readdirres) > 0) {
        nfs__read_dir_res__free_unpacked(readdirres, NULL);

        printf("Error: Invalid NFS procedure result received from the server\n");

        return;
    }

    if(readdirres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(readdirres->nfs_status->stat);
        printf("Error: Failed to read entries in cwd with status %s\n", string_status);
        free(string_status);

        nfs__read_dir_res__free_unpacked(readdirres, NULL);

        return;
    }

    Nfs__DirectoryEntriesList *entries = readdirres->readdirok->entries;
    while(entries != NULL) {
        printf("%s", entries->name->filename);
        entries = entries->nextentry;
        if(entries != NULL) {
            printf(" ");
        }
    }
    printf("\n");
    fflush(stdout);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}