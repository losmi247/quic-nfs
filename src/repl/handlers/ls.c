#include "handlers.h"

#define READDIR_BYTES_PER_RPC 50

typedef struct FileNamesList {
    char *filename;
    struct FileNamesList *next;
} FileNamesList;

/*
 * If there is a directory that is currently mounted, prints out all directory entries in the current
 * working directory.
 *
 * Returns 0 on success and > 0 on failure.
 */
int handle_ls() {
    if (!is_filesystem_mounted()) {
        printf("Error: No remote file system is currently mounted\n");
        return 1;
    }

    FileNamesList *filenames_list_head = NULL;
    FileNamesList *filenames_list_tail = NULL;

    uint64_t offset_cookie = 0;
    int read_all_directory_entries = 0;
    while (!read_all_directory_entries) {
        Nfs__NfsCookie nfs_cookie = NFS__NFS_COOKIE__INIT;
        nfs_cookie.value = offset_cookie;

        Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
        readdirargs.dir = cwd_node->fhandle;
        readdirargs.cookie = &nfs_cookie;
        readdirargs.count = READDIR_BYTES_PER_RPC;

        Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
        int status = nfs_procedure_16_read_from_directory(rpc_connection_context, readdirargs, readdirres);
        if (status != 0) {
            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            free(readdirres);

            return 1;
        }

        if (validate_nfs_read_dir_res(readdirres) > 0) {
            printf("Error: Invalid NFS procedure result received from the server\n");

            nfs__read_dir_res__free_unpacked(readdirres, NULL);

            return 1;
        }

        if (readdirres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("ls: Permission denied\n");

            nfs__read_dir_res__free_unpacked(readdirres, NULL);

            return 1;
        } else if (readdirres->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(readdirres->nfs_status->stat);
            printf("Error: Failed to read entries in cwd with status %s\n", string_status);
            free(string_status);

            nfs__read_dir_res__free_unpacked(readdirres, NULL);

            return 1;
        }

        // remember all found directory entries
        Nfs__DirectoryEntriesList *entries = readdirres->readdirok->entries;
        while (entries != NULL) {
            // add the filename to the end of the list
            FileNamesList *new_filenames_list_entry = malloc(sizeof(FileNamesList));
            new_filenames_list_entry->filename = strdup(entries->name->filename);
            new_filenames_list_entry->next = NULL;
            if (filenames_list_tail == NULL) {
                filenames_list_head = filenames_list_tail = new_filenames_list_entry;
            } else {
                filenames_list_tail->next = new_filenames_list_entry;
                filenames_list_tail = new_filenames_list_entry;
            }

            // update the UNIX directory stream offset cookie
            offset_cookie = entries->cookie->value;

            entries = entries->nextentry;
        }

        if (readdirres->readdirok->eof) {
            read_all_directory_entries = 1;
        }

        nfs__read_dir_res__free_unpacked(readdirres, NULL);
    }

    // print all file names in this directory
    FileNamesList *filenames_list = filenames_list_head;
    while (filenames_list != NULL) {
        printf("%s", filenames_list->filename);

        filenames_list = filenames_list->next;
        if (filenames_list != NULL) {
            printf(" ");
        }
    }
    printf("\n");
    fflush(stdout);

    // clean up the filenames list
    while (filenames_list_head != NULL) {
        FileNamesList *next = filenames_list_head->next;

        free(filenames_list_head->filename);
        free(filenames_list_head);

        filenames_list_head = next;
    }

    return 0;
}