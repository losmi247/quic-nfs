#include "readdir_sessions.h"

/*
* Creates a deep copy of the given AuthSysParams.
*
* The user of this function takes the responsibility to free the returned deep copy.
*/
Rpc__AuthSysParams *deep_copy_authsysparams(Rpc__AuthSysParams *authsysparams) {
    Rpc__AuthSysParams *deep_copy = malloc(sizeof(Rpc__AuthSysParams));
    rpc__auth_sys_params__init(deep_copy);

    deep_copy->timestamp = authsysparams->timestamp;
    deep_copy->machinename = strdup(authsysparams->machinename);
    deep_copy->uid = authsysparams->uid;
    deep_copy->gid = authsysparams->gid;

    deep_copy->n_gids = authsysparams->n_gids;
    deep_copy->gids = malloc(sizeof(uint32_t) * deep_copy->n_gids);
    for(int i = 0; i < deep_copy->n_gids; i++) {
        *(deep_copy->gids + i) = *(authsysparams->gids + i);
    }

    return deep_copy;
}

/*
* Frees all heap-allocated fields in the given AuthSysParams.
*
* Does nothing if the given AuthSysParams is NULL.
*/
void free_authsysparams(Rpc__AuthSysParams *authsysparams) {
    if(authsysparams == NULL) {
        return;
    }

    free(authsysparams->machinename);
    free(authsysparams->gids);
    free(authsysparams);
}

/*
* Creates a ReadDirSession for the given client and the given directory (given by its absolute path
* and its inode number), and opens its directory stream.
*
* The user of this function takes the responsibility to free the heap-allocated state in the returned
* ReadDirSession using 'free_readdir_session'.
*/
ReadDirSession *create_new_readdir_session(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path, ino_t directory_inode_number) {
    if(client_authsysparams == NULL) {
        fprintf(stderr, "create_new_readdir_session: client AuthSysParams is NULL\n");
        return NULL;
    }

    ReadDirSession *readdir_session = malloc(sizeof(ReadDirSession));
    if(readdir_session == NULL) {
        perror_msg("create_new_readdir_session: failed to allocate memory\n");
        return NULL;
    }

    readdir_session->client_authsysparams = deep_copy_authsysparams(client_authsysparams);

    readdir_session->directory_absolute_path = strdup(directory_absolute_path);
    readdir_session->directory_inode_number = directory_inode_number;

    readdir_session->directory_stream = opendir(directory_absolute_path);
    if(readdir_session->directory_stream == NULL) {
        perror_msg("create_new_readdir_session: failed to open the directory at absolute path %s", directory_absolute_path);

        free_authsysparams(readdir_session->client_authsysparams);
        free(readdir_session->directory_absolute_path);
        free(readdir_session);

        return NULL;
    }

    readdir_session->last_readdir_rpc_timestamp = time(NULL);

    return readdir_session;
}

/*
* Given a ReadDirSession, closes its directory stream, deallocates all heap-allocated fields in it,
* and frees the session itself.
*
* Does nothing if the given ReadDirSession is NULL.
*/
void free_readdir_session(ReadDirSession *readdir_session) {
    if(readdir_session == NULL) {
        return;
    }

    if(closedir(readdir_session->directory_stream) < 0) {
        perror_msg("free_readdir_session: failed closing the directory stream of directory at absolute path %s\n", readdir_session->directory_absolute_path);
    }

    free_authsysparams(readdir_session->client_authsysparams);
    free(readdir_session->directory_absolute_path);

    free(readdir_session);
}

/*
* Creates a fresh ReadDir session for the given client to read the given 
* directory, and adds it to the head of the given list of ReadDir sessions.
*
* Returns 0 on success and > 0 on failure.
*
* The user of this function takes the responsibility to free the ReadDir sessions added
* to the list using 'remove_readdir_session'.
*/
int add_new_readdir_session(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path, ino_t directory_inode_number, ReadDirSessionsList **readdir_sessions) {
    if(readdir_sessions == NULL) {
        fprintf(stderr, "add_new_readdir_session: list of ReadDir sessions is NULL\n");
        return 1;
    }

    ReadDirSession *new_readdir_session = create_new_readdir_session(client_authsysparams, directory_absolute_path, directory_inode_number);
    if(new_readdir_session == NULL) {
        return 2;
    }

    ReadDirSessionsList *new_head = malloc(sizeof(ReadDirSessionsList));
    if(new_head == NULL) {
        fprintf(stderr, "add_new_readdir_session: failed to allocate memory\n");

        free_readdir_session(new_readdir_session);

        return 3;
    }
    new_head->readdir_session = new_readdir_session;
    new_head->next = *readdir_sessions;

    *readdir_sessions = new_head;

    return 0;
}

/*
* Returns 0 if the two given AuthSysParams correspond to the same user, and 1 otherwise.
*/
int compare_authsysparams(Rpc__AuthSysParams *authsysparams1, Rpc__AuthSysParams *authsysparams2) {
    if(authsysparams1 == NULL) {
        fprintf(stderr, "compare_authsysparams: the first AuthSysParams is NULL\n");
        return 1;
    }
    if(authsysparams2 == NULL) {
        fprintf(stderr, "compare_authsysparams: the second AuthSysParams is NULL\n");
        return 1;
    }

    if(strcmp(authsysparams1->machinename, authsysparams2->machinename) != 0) {
        return 1;
    }
    if(authsysparams1->uid != authsysparams2->uid) {
        return 1;
    }
    if(authsysparams2->gid != authsysparams2->gid) {
        return 1;
    }

    return 0;
}

/*
* Checks if the ReadDir session for the given client and the directory given by its inode number is present
* inside the given list of ReadDir sessions.
*
* Returns 0 if the ReadDir session for this client and directory was found in the list, and 1 otherwise.
*/
int find_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number, ReadDirSessionsList *readdir_sessions) {
    if(client_authsysparams == NULL) {
        fprintf(stderr, "find_readdir_session: client AuthSysParams is NULL\n");
        return 1;
    }

    ReadDirSessionsList *curr = readdir_sessions;
    while(curr != NULL) {
        ReadDirSession *readdir_session = curr->readdir_session;
        if(readdir_session == NULL) {
            fprintf(stderr, "find_readdir_session: encountered a NULL ReadDir session in the list\n");

            curr = curr->next;

            continue;
        }
        Rpc__AuthSysParams *session_authsysparams = readdir_session->client_authsysparams;
        if(session_authsysparams == NULL) {
            fprintf(stderr, "find_readdir_session: encountered a ReadDirSession with NULL 'client_authsysparams' in the list\n");

            curr = curr->next;

            continue;
        }

        if(readdir_session->directory_inode_number == directory_inode_number && compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
            return 0;
        }

        curr = curr->next;
    }

    return 1;
}

/*
* Finds the ReadDir session for the given client and the directory given by its inode number inside the given list 
* of ReadDir sessions, and returns that ReadDir session.
*
* Returns NULL on failure (e.g. session for this client and directory not found).
*/
ReadDirSession *get_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number, ReadDirSessionsList *readdir_sessions) {
    if(client_authsysparams == NULL) {
        fprintf(stderr, "get_readdir_session: client AuthSysParams is NULL\n");
        return NULL;
    }
    
    ReadDirSessionsList *curr = readdir_sessions;
    while(curr != NULL) {
        ReadDirSession *readdir_session = curr->readdir_session;
        if(readdir_session == NULL) {
            fprintf(stderr, "get_readdir_session: encountered a NULL ReadDir session in the list\n");
            return NULL;
        }
        Rpc__AuthSysParams *session_authsysparams = readdir_session->client_authsysparams;
        if(session_authsysparams == NULL) {
            fprintf(stderr, "get_readdir_session: encountered a ReadDirSession with NULL 'client_authsysparams' in the list\n");

            curr = curr->next;

            continue;
        }

        if(readdir_session->directory_inode_number == directory_inode_number && compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
            return readdir_session;
        }

        curr = curr->next;
    }

    return NULL;
}

/*
* Locates the ReadDir session for the given client and the directory (given by its inode number)
* inside the given list of ReadDir sessions, removes it from that list, and frees that ReadDir session.
*
* Returns 0 on success and > 0 on failure (e.g. session for this client and directory not found).
*/
int remove_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number, ReadDirSessionsList **readdir_sessions) {
    if(client_authsysparams == NULL) {
        fprintf(stderr, "remove_readdir_session: client AuthSysParams is NULL\n");
        return 1;
    }

    if(readdir_sessions == NULL) {
        fprintf(stderr, "remove_readdir_session: ReadDir session not found\n");
        return 2;
    }

    // the entry we want to remove is the first in the list
    ReadDirSession *first_session = (*readdir_sessions)->readdir_session;
    if(first_session == NULL) {
        fprintf(stderr, "remove_readdir_session: encountered a NULL ReadDir session in the list\n");
        return 3;
    }
    Rpc__AuthSysParams *session_authsysparams = first_session->client_authsysparams;
    if(session_authsysparams == NULL) {
        fprintf(stderr, "remove_readdir_session: encountered a ReadDirSession with NULL 'client_authsysparams' in the list\n");
        return 4;
    }
    if(first_session->directory_inode_number == directory_inode_number && compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
        ReadDirSessionsList *new_head = (*readdir_sessions)->next;

        free_readdir_session(first_session);
        free(*readdir_sessions);

        *readdir_sessions = new_head;

        return 0;
    }

    // the entry we want to remove is somewhere in the middle of the list
    ReadDirSessionsList *curr = (*readdir_sessions)->next, *prev = *readdir_sessions;
    while(curr != NULL) {
        ReadDirSession *readdir_session = curr->readdir_session;
        if(readdir_session == NULL) {
            fprintf(stderr, "remove_readdir_session: encountered a NULL ReadDir session in the list\n");
            return 5;
        }
        session_authsysparams = readdir_session->client_authsysparams;
        if(readdir_session->directory_inode_number == directory_inode_number && compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
            prev->next = curr->next;

            free_readdir_session(readdir_session);
            free(curr);

            return 0;
        }

        prev = curr;
        curr = curr->next;
    }

    fprintf(stderr, "remove_readdir_session: ReadDir session not found\n");
    return 6;
}

/*
* Clears all ReadDir sessions in the given list.
*/
void clean_up_readdir_sessions_list(ReadDirSessionsList *readdir_sessions) {
    while(readdir_sessions != NULL) {
        ReadDirSessionsList *next = readdir_sessions->next;

        free_readdir_session(readdir_sessions->readdir_session);
        free(readdir_sessions);

        readdir_sessions = next;
    }
}
