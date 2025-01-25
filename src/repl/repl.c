#include "repl.h"

#include "handlers/handlers.h"

#define BUFFER_SIZE 256
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"    // red color for displaying the CWD
#define KGRN  "\x1B[32m"
#define KYLW  "\x1B[33m"
#define KBLU  "\x1B[34m"

#define UP_ARROW 65
#define DOWN_ARROW 66
#define ENTER_KEY 10

/*
* Define Nfs Client state.
*/

RpcConnectionContext *rpc_connection_context;

DAGNode *filesystem_dag_root;
DAGNode *cwd_node;

TransportProtocol chosen_transport_protocol = TRANSPORT_PROTOCOL_TCP;

int is_filesystem_mounted(void) {
    return rpc_connection_context != NULL && filesystem_dag_root != NULL && cwd_node != NULL;
}

/*
* Functions used by the REPL body.
*/

/*
* Lists all available commands.
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

    printf(KBLU "touch <file name>                               ");
    printf(KNRM "- create a file in the current working directory\n");
    printf(KBLU "mkdir <directory name>                          ");
    printf(KNRM "- create a directory in the current working directory\n");

    printf(KBLU "cat <file name>                                 ");
    printf(KNRM "- print out contents of a file in the current working directory\n");
    printf(KBLU "echo '<text>' >> <file name>                      ");
    printf(KNRM "- appends the given text to the file in the current working directory\n");


    printf("\nType ");
    printf(KBLU "exit");
    printf(KNRM " to quit the REPL.\n\n");
}

void print_transport_protocol_menu_choices(const char *choices[], int num_choices, int highlighted_choice) {
    printf("Use arrow keys to select the transport protocol, and press Enter to select:\n");

    for(int i = 0; i < num_choices; i++) {
        if(i == highlighted_choice) {
            printf(KRED "%s" KNRM "\n", choices[i]); 
        }
        else {
            printf(KNRM "%s\n", choices[i]);
        }
    }

    // move the terminal's cursor to the start of the highlighted choice
    printf("\033[%dA", num_choices - highlighted_choice);
}

void clear_transport_protocol_menu(int num_choices, int highlighted_choice) {
    // move the terminal's cursor to the top of the menu area
    printf("\033[%dA", highlighted_choice + 1);

    for(int i = 0; i < num_choices + 1; i++) {
        // clear the line and move down
        printf("\033[K\n");
    }

    // move the terminal's cursor to the top of the menu area
    printf("\033[%dA", num_choices + 1);
}

/*
* Allos the user select between TCP and QUIC as transport protocols.
*/
int display_transport_protocol_menu() {
    struct termios oldt, newt;
    int highlighted_choice = 0;

    int num_choices = 2;
    const char *choices[] = {"TCP", "QUIC"};

    // save old terminal settings and configure new settings for raw mode
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);           // disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    print_transport_protocol_menu_choices(choices, num_choices, highlighted_choice);

    while(1) {
        int ch = getchar();

        clear_transport_protocol_menu(num_choices, highlighted_choice);

        if (ch == '\033') { // only process escape sequences
            getchar();      // skip the '['
            switch(getchar()) {
                case UP_ARROW:
                    highlighted_choice = highlighted_choice > 0 ? highlighted_choice - 1 : 0;
                    break;
                case DOWN_ARROW:
                    highlighted_choice = highlighted_choice < num_choices-1 ? highlighted_choice + 1 : highlighted_choice;
                    break;
            }
        }
        else if(ch == ENTER_KEY) {
            chosen_transport_protocol = (highlighted_choice == 0 ? TRANSPORT_PROTOCOL_TCP : TRANSPORT_PROTOCOL_QUIC);
            break;
        }

        print_transport_protocol_menu_choices(choices, num_choices, highlighted_choice);
    }

    // clear the menu and restore the terminal
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return 0;
}

/*
* Displays the CWD and '>' at the start of the line in the REPL.
*/
void display_prompt(void) {
    char *transport_protocol = chosen_transport_protocol == TRANSPORT_PROTOCOL_TCP ? "TCP" : "QUIC";
    printf(KYLW "{%s}" KNRM " ", transport_protocol);

    // CWD information
    if(!is_filesystem_mounted()) {
        printf(KRED "(no NFS share mounted)");
    }
    else {
        printf(KRED "(%s:%s)", rpc_connection_context->server_ipv4_addr, cwd_node->absolute_path);
    }
    
    printf(KGRN " >" KNRM " ");

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
    display_transport_protocol_menu();

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

            int error_code = handle_mount(server_ip, port_number, remote_path);
        }
        else if(strcmp(input, "ls") == 0) {
            int error_code = handle_ls();
        }
        else if(strncmp(input, "cd", 2) == 0) {
            char directory_name[BUFFER_SIZE];
            int arguments_passed = sscanf(input + 2, " %s", directory_name);

            if(arguments_passed != 1) {
                printf("Error: Invalid 'cd' command. Correct usage: cd <directory name>\n");
                continue;
            }

            int error_code = handle_cd(directory_name);
        }
        else if(strncmp(input, "touch", 5) == 0) {
            char file_name[BUFFER_SIZE];
            int arguments_passed = sscanf(input + 5, " %s", file_name);

            if(arguments_passed != 1) {
                printf("Error: Invalid 'touch' command. Correct usage: touch <file name>\n");
                continue;
            }

            int error_code = handle_touch(file_name);
        }
        else if(strncmp(input, "mkdir", 5) == 0) {
            char directory_name[BUFFER_SIZE];
            int arguments_passed = sscanf(input + 5, " %s", directory_name);

            if(arguments_passed != 1) {
                printf("Error: Invalid 'mkdir' command. Correct usage: mkdir <directory name>\n");
                continue;
            }

            int error_code = handle_mkdir(directory_name);
        }
        else if(strncmp(input, "cat", 3) == 0) {
            char file_name[BUFFER_SIZE];
            int arguments_passed = sscanf(input + 3, " %s", file_name);

            if(arguments_passed != 1) {
                printf("Error: Invalid 'cat' command. Correct usage: cat <file name>\n");
                continue;
            }

            int error_code = handle_cat(file_name);
        }
        else if(strncmp(input, "echo", 4) == 0) {
            char text[BUFFER_SIZE], file_name[BUFFER_SIZE];
            int arguments_parsed = sscanf(input + 4, " '%[^']' >> %s", text, file_name);

            if(arguments_parsed != 2) {
                printf("Error: Invalid 'echo' command. Correct usage: echo '<text>' >> <file name>\n");
                continue;
            }

            int error_code = handle_echo(text, file_name);
        }
        else {
            printf("Unrecognized command: '%s'\n", input);
        }
    }

    return 0;
}
