#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "repl.h"

#include "handlers/handlers.h"

#define BUFFER_SIZE 256
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"    // red color for displaying the CWD
#define KGRN  "\x1B[32m"

/*
* Define Nfs Client state.
*/

char *server_ipv4_addr;
uint16_t server_port_number;

char *cwd_absolute_path;
Nfs__FHandle *cwd_filehandle;

/*
* Functions used by the REPL body.
*/

void display_prompt() {
    if(cwd_absolute_path == NULL) {
        printf(KRED "(no NFS share mounted)");
    }
    else {
        printf(KRED "(%s:%s)", server_ipv4_addr, cwd_absolute_path);
    }
    
    printf(KGRN " >");
    printf(KNRM " ");   // so that user input hass normal colour

    fflush(stdout);
}

int main(void) {
    printf("NFS Client REPL\n");
    printf("Supported commands:\n\n");
    
    printf("mount <server_ip> <port> <remote_path>          - mount a remote NFS share\n");
    printf("ls                                              - list all entries in the current working directory\n");
    printf("cd <directory name>                             - change cwd to the given directory name which is in the current working directory\n");

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
        if(strncmp(input, "mount", 5) == 0) {
            char server_ip[BUFFER_SIZE], port_number_string[BUFFER_SIZE], remote_path[BUFFER_SIZE];
            int arguments_parsed = sscanf(input + 5, " %s %s %s", server_ip, port_number_string, remote_path);

            if(arguments_parsed != 3) {
                printf("Error: Invalid 'mount' command. Correct usage: mount <server_ip> <port> <remote_path>\n");
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
        else if(strncmp(input, "cd", 2) == 0) {
            char directory_name[BUFFER_SIZE];
            int arguments_passed = sscanf(input + 2, " %s", directory_name);

            if(arguments_passed != 1) {
                printf("Error: Invalid 'cd' command. Correct usage: cd <directory name>\n");
                continue;
            }

            handle_cd(directory_name);
        }
        else {
            printf("Unrecognized command: '%s'\n", input);
        }
    }

    return 0;
}
