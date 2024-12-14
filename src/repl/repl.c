#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

#include "src/parsing/parsing.h"
#include "filehandle_management.h"

#define BUFFER_SIZE 256

/*
* Nfs Client state.
*/

char *server_ipv4_addr;
uint16_t server_port_number;

char *cwd_absolute_path;
Nfs__FHandle *cwd_filehandle;


void display_prompt() {
    if(cwd_absolute_path == NULL) {
        printf("(no NFS share mounted) > ");
    }
    else {
        printf("(%s:%s) > ", server_ipv4_addr, cwd_absolute_path);
    }
    fflush(stdout);
}

/*
* *NIX-like file management command interpreter functions.
*/

/*
* Given the IPv4 address and port number of a NFS server and an absolute path of a directory at the device
* where the server resides, attempts to mount this directory via NFS.
*
* Replaces any directories (from same or another NFS server) mounted so far with this new one, and sets the
* current working directory to the mounted directory.
*/
void handle_mount(const char *server_ip, uint16_t server_port, char *remote_path) {
    // clean up any previously mounted directories
    if(cwd_absolute_path != NULL) {
        free(server_ipv4_addr);
        free(cwd_filehandle->nfs_filehandle);
        free(cwd_absolute_path);

        cwd_filehandle = NULL;
        cwd_absolute_path = NULL;
        server_ipv4_addr = NULL;
        server_port_number = 0;
    }

    printf("Mounting NFS share...\n");
    printf("Server IP: %s\n", server_ip);
    printf("Port: %d\n", server_port);
    printf("Remote Path: %s\n\n", remote_path);

    // mount the new NFS share
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = remote_path;

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(server_ip, server_port, dirpath, fhstatus);
    if(status != 0) {
        free(fhstatus);

        printf("Error: Failed to mount the NFS share with status %d\n", status);

        return;
    }

    if(fhstatus->mnt_status->stat != MOUNT__STAT__MNT_OK) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        char *string_status = nfs_stat_to_string(fhstatus->mnt_status->stat);
        printf("Error: Failed to mount the NFS share with status %s\n", string_status);
        free(string_status);

        return;
    }

    // set the client state
    server_ipv4_addr = strdup(server_ip);
    server_port_number = server_port;
    cwd_absolute_path = strdup(remote_path);

    NfsFh__NfsFileHandle *nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    *nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    cwd_filehandle->nfs_filehandle = nfs_filehandle_copy;
}

/*
* If there is a directory that is currently mounted, prints out all directory entries in the current
* working directory.
*/
void handle_ls() {
    if(cwd_absolute_path == NULL) {
        printf("Error: No remote file system is mounted\n");
        return;
    }

    Nfs__NfsCookie nfs_cookie = NFS__NFS_COOKIE__INIT;
    nfs_cookie.value = 0;

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = cwd_filehandle;
    readdirargs.cookie = &nfs_cookie;
    readdirargs.count = 1000;   // try to read all entries

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(server_ipv4_addr, server_port_number, readdirargs, readdirres);
    if(status != 0) {
        free(readdirres);

        printf("Error: Failed to read entries in cwd with status %d\n", status);

        return;
    }

    if(readdirres->nfs_status->stat != NFS__STAT__NFS_OK) {
        nfs__read_dir_res__free_unpacked(readdirres, NULL);

        char *string_status = nfs_stat_to_string(readdirres->nfs_status->stat);
        printf("Error: Failed to read entries in cwd with status %s\n", string_status);
        free(string_status);

        return;
    }

    Nfs__DirectoryEntriesList *entries = readdirres->readdirok->entries;
    while(entries != NULL) {
        printf("%s ", entries->name->filename);
        entries = entries->nextentry;
    }
    printf("\n");
    fflush(stdout);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

int main(void) {
    printf("NFS Client REPL\n");
    printf("Supported commands:\n\n");
    
    printf("mount <server_ip> <port> <remote_path>          - mount a remote NFS share\n");
    printf("ls                                              - list all entries in the current working directory\n");

    printf("\nType 'exit' to quit the REPL.\n\n");

    // initialize NFS client state
    server_ipv4_addr = NULL;
    server_port_number = 0;
    cwd_absolute_path = NULL;
    cwd_filehandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(cwd_filehandle);

    // Read-Eval-Print Loop
    char input[BUFFER_SIZE];
    while(1) {
        display_prompt();

        // read user input
        if(!fgets(input, BUFFER_SIZE, stdin)) {
            perror("Error reading input");
            return 1;
        }

        // remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // check for exit condition
        if(strcmp(input, "exit") == 0) {
            printf("Exiting REPL. Goodbye! \n");
            break;
        }

        // parse commands
        if(strncmp(input, "mount ", 6) == 0) {
            char server_ip[BUFFER_SIZE], port_number_string[BUFFER_SIZE], remote_path[BUFFER_SIZE];
            int arguments_parsed = sscanf(input + 6, "%s %s %s", server_ip, port_number_string, remote_path);

            if(arguments_parsed != 3) {
                printf("Error: Invalid mount command. Correct usage: mount <server_ip> <port> <remote_path>\n");
                continue;
            }

            uint16_t port_number = parse_port_number(port_number_string);
            if(port_number == 0) {
                printf("Error: Invalid port number. Correct usage: mount <server_ip> <port> <remote_path>\n");
                continue;
            }

            handle_mount(server_ip, port_number, remote_path);
        }
        else if(strcmp(input, "ls") == 0) {
            handle_ls();
        }
        else {
            printf("Unrecognized command: '%s'\n", input);
        }
    }

    return 0;
}
