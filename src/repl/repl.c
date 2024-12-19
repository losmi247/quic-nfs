#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "repl.h"

#include "handlers/handlers.h"

#define BUFFER_SIZE 256
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"    // red color for displaying the CWD
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"

/*
* Define Nfs Client state.
*/

RpcConnectionContext *rpc_connection_context;

DAGNode *filesystem_dag_root;
DAGNode *cwd_node;


int is_filesystem_mounted(void) {
    return rpc_connection_context != NULL && filesystem_dag_root != NULL && cwd_node != NULL;
}

/*
* Functions used by the REPL body.
*/

void display_introduction(void) {
    printf("NFS Client REPL\n");
    printf("Supported commands:\n\n");
    
    printf(KBLU "mount <server_ip> <port> <remote_path>          ");
    printf(KNRM "- mount a remote NFS share\n");

    printf(KBLU "ls                                              ");
    printf(KNRM "- list all entries in the current working directory\n");

    printf(KBLU "cd <directory name>                             ");
    printf(KNRM "- change cwd to the given directory name which is in the current working directory\n");

    printf("\nType ");
    printf(KBLU "exit");
    printf(KNRM " to quit the REPL.\n\n");
}

/*
* Displays the CWD and '>' at the start of the line in the REPL.
*/
void display_prompt(void) {
    if(!is_filesystem_mounted()) {
        printf(KRED "(no NFS share mounted)");
    }
    else {
        printf(KRED "(%s:%s)", rpc_connection_context->server_ipv4_addr, cwd_node->absolute_path);
    }
    
    printf(KGRN " >");
    printf(KNRM " ");   // so that user input hass normal colour

    fflush(stdout);
}

/*
* Cleans up all Nfs client state before the REPL shuts down.
*/
void clean_up(void) {
    free_rpc_connection_context(rpc_connection_context);

    clean_up_dag(filesystem_dag_root);
}

int main(void) {
    display_introduction();

    // initialize NFS client state
    rpc_connection_context = NULL;
    filesystem_dag_root = NULL;
    cwd_node = NULL;

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
            clean_up();

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
