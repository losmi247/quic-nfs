#include "directory_reading.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "src/error_handling/error_handling.h"

#define READDIR_SESSION_TIMEOUT 30 // a ReadDir session expires after not being touched for this many seconds

pthread_mutex_t readdir_sessions_mutex =
    PTHREAD_MUTEX_INITIALIZER; // mutex for protecting concurrent access to any ReadDirSessionsList

/*
 * Creates a deep copy of the given AuthSysParams.
 *
 * The user of this function takes the responsibility to free the returned deep copy
 * using 'free_authsysparams' function.
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
    for (int i = 0; i < deep_copy->n_gids; i++) {
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
    if (authsysparams == NULL) {
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
ReadDirSession *create_new_readdir_session(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path,
                                           ino_t directory_inode_number) {
    if (client_authsysparams == NULL) {
        fprintf(stderr, "create_new_readdir_session: client AuthSysParams is NULL\n");
        return NULL;
    }

    ReadDirSession *readdir_session = malloc(sizeof(ReadDirSession));
    if (readdir_session == NULL) {
        perror_msg("create_new_readdir_session: failed to allocate memory\n");
        return NULL;
    }

    readdir_session->client_authsysparams = deep_copy_authsysparams(client_authsysparams);

    readdir_session->directory_absolute_path = strdup(directory_absolute_path);
    readdir_session->directory_inode_number = directory_inode_number;

    readdir_session->directory_stream = opendir(directory_absolute_path);
    if (readdir_session->directory_stream == NULL) {
        perror_msg("create_new_readdir_session: failed to open the directory at absolute path %s",
                   directory_absolute_path);

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
    if (readdir_session == NULL) {
        return;
    }

    if (closedir(readdir_session->directory_stream) < 0) {
        perror_msg("free_readdir_session: failed closing the directory stream of directory at absolute path %s\n",
                   readdir_session->directory_absolute_path);
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
int add_new_readdir_session(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path,
                            ino_t directory_inode_number, ReadDirSessionsList **readdir_sessions) {
    if (readdir_sessions == NULL) {
        fprintf(stderr, "add_new_readdir_session: list of ReadDir sessions is NULL\n");
        return 1;
    }

    ReadDirSession *new_readdir_session =
        create_new_readdir_session(client_authsysparams, directory_absolute_path, directory_inode_number);
    if (new_readdir_session == NULL) {
        return 2;
    }

    ReadDirSessionsList *new_head = malloc(sizeof(ReadDirSessionsList));
    if (new_head == NULL) {
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
    if (authsysparams1 == NULL) {
        fprintf(stderr, "compare_authsysparams: the first AuthSysParams is NULL\n");
        return 1;
    }
    if (authsysparams2 == NULL) {
        fprintf(stderr, "compare_authsysparams: the second AuthSysParams is NULL\n");
        return 1;
    }

    if (strcmp(authsysparams1->machinename, authsysparams2->machinename) != 0) {
        return 1;
    }
    if (authsysparams1->uid != authsysparams2->uid) {
        return 1;
    }
    if (authsysparams2->gid != authsysparams2->gid) {
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
int find_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number,
                         ReadDirSessionsList *readdir_sessions) {
    if (client_authsysparams == NULL) {
        fprintf(stderr, "find_readdir_session: client AuthSysParams is NULL\n");
        return 1;
    }

    ReadDirSessionsList *curr = readdir_sessions;
    while (curr != NULL) {
        ReadDirSession *readdir_session = curr->readdir_session;
        if (readdir_session == NULL) {
            fprintf(stderr, "find_readdir_session: encountered a NULL ReadDir session in the list\n");

            curr = curr->next;

            continue;
        }
        Rpc__AuthSysParams *session_authsysparams = readdir_session->client_authsysparams;
        if (session_authsysparams == NULL) {
            fprintf(
                stderr,
                "find_readdir_session: encountered a ReadDirSession with NULL 'client_authsysparams' in the list\n");

            curr = curr->next;

            continue;
        }

        if (readdir_session->directory_inode_number == directory_inode_number &&
            compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
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
 * This function executes atomically, using the 'readdir_sessions_mutex'.
 *
 * Returns NULL on failure (e.g. session for this client and directory not found).
 */
ReadDirSession *get_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number,
                                    ReadDirSessionsList *readdir_sessions) {
    if (client_authsysparams == NULL) {
        fprintf(stderr, "get_readdir_session: client AuthSysParams is NULL\n");
        return NULL;
    }

    ReadDirSessionsList *curr = readdir_sessions;
    while (curr != NULL) {
        ReadDirSession *readdir_session = curr->readdir_session;
        if (readdir_session == NULL) {
            fprintf(stderr, "get_readdir_session: encountered a NULL ReadDir session in the list\n");

            curr = curr->next;

            continue;
        }
        Rpc__AuthSysParams *session_authsysparams = readdir_session->client_authsysparams;
        if (session_authsysparams == NULL) {
            fprintf(stderr,
                    "get_readdir_session: encountered a ReadDirSession with NULL 'client_authsysparams' in the list\n");

            curr = curr->next;

            continue;
        }

        if (readdir_session->directory_inode_number == directory_inode_number &&
            compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
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
 * This function executes atomically, using the 'readdir_sessions_mutex'.
 *
 * Returns 0 on success and > 0 on failure (e.g. session for this client and directory not found).
 */
int remove_readdir_session(Rpc__AuthSysParams *client_authsysparams, ino_t directory_inode_number,
                           ReadDirSessionsList **readdir_sessions) {
    if (client_authsysparams == NULL) {
        fprintf(stderr, "remove_readdir_session: client AuthSysParams is NULL\n");
        return 1;
    }

    if (readdir_sessions == NULL) {
        fprintf(stderr, "remove_readdir_session: ReadDir session not found\n");
        return 2;
    }

    // the entry we want to remove is the first in the list
    ReadDirSession *first_session = (*readdir_sessions)->readdir_session;
    if (first_session == NULL) {
        fprintf(stderr, "remove_readdir_session: encountered a NULL ReadDir session in the list\n");
        return 3;
    }
    Rpc__AuthSysParams *session_authsysparams = first_session->client_authsysparams;
    if (session_authsysparams == NULL) {
        fprintf(stderr,
                "remove_readdir_session: encountered a ReadDirSession with NULL 'client_authsysparams' in the list\n");
        return 4;
    }
    if (first_session->directory_inode_number == directory_inode_number &&
        compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
        ReadDirSessionsList *new_head = (*readdir_sessions)->next;

        free_readdir_session(first_session);
        free(*readdir_sessions);

        *readdir_sessions = new_head;

        return 0;
    }

    // the entry we want to remove is somewhere in the middle of the list
    ReadDirSessionsList *curr = (*readdir_sessions)->next, *prev = *readdir_sessions;
    while (curr != NULL) {
        ReadDirSession *readdir_session = curr->readdir_session;
        if (readdir_session == NULL) {
            fprintf(stderr, "remove_readdir_session: encountered a NULL ReadDir session in the list\n");

            curr = curr->next;

            continue;
        }
        session_authsysparams = readdir_session->client_authsysparams;
        if (readdir_session->directory_inode_number == directory_inode_number &&
            compare_authsysparams(client_authsysparams, session_authsysparams) == 0) {
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
    while (readdir_sessions != NULL) {
        ReadDirSessionsList *next = readdir_sessions->next;

        free_readdir_session(readdir_sessions->readdir_session);
        free(readdir_sessions);

        readdir_sessions = next;
    }
}

/*
 * Iterates through the given list of ReadDir session and removes and deallocates ReadDir sessions
 * which were read from more than 'timeout' seconds ago.
 *
 * This function executes atomically, using the 'readdir_sessions_mutex'.
 */
void clean_up_expired_readdir_sessions(ReadDirSessionsList **readdir_sessions) {
    if (readdir_sessions == NULL) {
        return;
    }

    pthread_mutex_lock(&readdir_sessions_mutex);

    time_t current_time = time(NULL);
    ReadDirSessionsList *prev = NULL, *curr = *readdir_sessions;
    while (curr != NULL) {
        ReadDirSession *readdir_session = curr->readdir_session;
        if (readdir_session == NULL) {
            fprintf(stderr, "clean_up_expired_readdir_sessions: encountered NULL ReadDir session in the list \n");
            continue;
        }

        if (difftime(current_time, readdir_session->last_readdir_rpc_timestamp) > READDIR_SESSION_TIMEOUT) {
            ReadDirSessionsList *next = curr->next;

            if (prev != NULL) {
                prev->next = curr->next;
            } else {
                *readdir_sessions = curr->next;
            }

            free_readdir_session(readdir_session);
            free(curr);

            curr = next;
        } else {
            prev = curr;
            curr = curr->next;
        }
    }

    pthread_mutex_unlock(&readdir_sessions_mutex);
}

void clean_up_directory_entries_list(Nfs__DirectoryEntriesList *directory_entries_list_head) {
    while (directory_entries_list_head != NULL) {
        Nfs__DirectoryEntriesList *next = directory_entries_list_head->nextentry;

        free(directory_entries_list_head->name->filename);
        free(directory_entries_list_head->name);
        free(directory_entries_list_head->cookie);

        free(directory_entries_list_head);

        directory_entries_list_head = next;
    }
}

/*
 * Given a client (by its AUTH_SYS parameters) and a directory by its absolute path and inode number,
 * fetches the ReadDir session for this client and directory (or creates a new one if no such session is
 * currently active) from the given collection of active ReadDir sessions ('active_read_dir_sessions' argument).
 * Then reads directory entries from it, creates a list of them, and places that list in the 'head' argument.
 * It will read as many directory entries as possible, such that their total size does not exceed 'byte_count'.
 * In case end of directory stream was reached, 'end_of_stream' is set to 1.
 *
 * This function executes atomically, using the 'readdir_sessions_mutex'.
 *
 * Returns 0 on success and > 0 on failure. The 'directory_absolute_path' is only used for printing in
 * case of an error.
 *
 * In case of successful execution, the user of this function takes the responsibility to free all
 * directory entries, Nfs__FileName's, Nfs__FileName.filename's, and Nfs__NfsCookie's inside them
 * using the clean_up_directory_entries_list() function.
 */
int read_from_directory(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path,
                        ino_t directory_inode_number, ReadDirSessionsList **active_readdir_sessions, long offset_cookie,
                        size_t byte_count, Nfs__DirectoryEntriesList **head, int *end_of_stream) {
    if (active_readdir_sessions == NULL) {
        fprintf(stderr, "read_from_directory: readdir_sessions is NULL\n");
        return 1;
    }

    if (client_authsysparams == NULL) {
        fprintf(stderr, "read_from_directory: client AuthSysParams is NULL\n");
        return 2;
    }

    pthread_mutex_lock(&readdir_sessions_mutex);

    if (find_readdir_session(client_authsysparams, directory_inode_number, *active_readdir_sessions) != 0) {
        // there is no active ReadDir session for this client and directory, so create one
        int error_code = add_new_readdir_session(client_authsysparams, directory_absolute_path, directory_inode_number,
                                                 active_readdir_sessions);
        if (error_code > 0) {
            fprintf(stderr, "read_from_directory: failed to create a new ReadDir session\n");

            pthread_mutex_unlock(&readdir_sessions_mutex);

            return 3;
        }
    }

    // get the ReadDir session for this client and directory
    ReadDirSession *readdir_session =
        get_readdir_session(client_authsysparams, directory_inode_number, *active_readdir_sessions);
    if (readdir_session == NULL) {
        fprintf(stderr, "read_from_directory: ReadDir session for client %s:%d:%d and directory %s not found\n",
                client_authsysparams->machinename, client_authsysparams->uid, client_authsysparams->gid,
                directory_absolute_path);

        pthread_mutex_unlock(&readdir_sessions_mutex);

        return 4;
    }

    // 0 is a special cookie value meaning client wants to start from beginning of directory stream
    if (offset_cookie == 0) {
        rewinddir(readdir_session->directory_stream);
    } else {
        // set the position within directory stream to the specified cookie
        seekdir(readdir_session->directory_stream, offset_cookie);
    }

    uint32_t total_size = 0;
    Nfs__DirectoryEntriesList *directory_entries_list_head = NULL, *directory_entries_list_tail = NULL;
    do {
        errno = 0;
        struct dirent *directory_entry =
            readdir(readdir_session->directory_stream); // man page of 'readdir' says not to free this
        if (directory_entry == NULL) {
            if (errno == 0) {
                // end of the directory stream reached
                *end_of_stream = 1;
                break;
            } else {
                perror_msg("Error occured while reading entries of directory at absolute path %s",
                           directory_absolute_path);

                // clean up the directory entries allocated so far
                clean_up_directory_entries_list(directory_entries_list_head);

                pthread_mutex_unlock(&readdir_sessions_mutex);

                return 5;
            }
        }

        // construct a new directory entry
        Nfs__DirectoryEntriesList *new_directory_entry = malloc(sizeof(Nfs__DirectoryEntriesList));
        nfs__directory_entries_list__init(new_directory_entry);
        new_directory_entry->fileid =
            directory_entry->d_ino; // fileid in FAttr is inode number, so this fileid should also be inode number

        Nfs__FileName *file_name = malloc(sizeof(Nfs__FileName));
        nfs__file_name__init(file_name);
        file_name->filename = strdup(
            directory_entry
                ->d_name); // make a copy of the file name to persist after this dirent is deallocated by closedir()
        new_directory_entry->name = file_name;

        long posix_cookie = telldir(readdir_session->directory_stream);
        if (posix_cookie < 0) {
            perror_msg("Failed getting current location in directory stream of directory at absolute path %s",
                       directory_absolute_path);

            // clean up the directory entries allocated so far
            clean_up_directory_entries_list(directory_entries_list_head);

            pthread_mutex_unlock(&readdir_sessions_mutex);

            return 6;
        }
        Nfs__NfsCookie *nfs_cookie = malloc(sizeof(Nfs__NfsCookie));
        nfs__nfs_cookie__init(nfs_cookie);
        nfs_cookie->value = posix_cookie;
        new_directory_entry->cookie = nfs_cookie;

        new_directory_entry->nextentry = NULL;

        // check we're not exceeding limit on bytes read, using Protobuf get_packed_size
        size_t directory_entry_packed_size = nfs__directory_entries_list__get_packed_size(new_directory_entry);
        if (total_size + directory_entry_packed_size > byte_count) {
            break;
        }
        total_size += directory_entry_packed_size;

        // append the new directory entry to the end of the list
        if (directory_entries_list_tail == NULL) {
            directory_entries_list_head = directory_entries_list_tail = new_directory_entry;
            // set 'head' argument to head of the list of entries
            *head = directory_entries_list_head;
        } else {
            directory_entries_list_tail->nextentry = new_directory_entry;
            directory_entries_list_tail = new_directory_entry;
        }
    } while (total_size < byte_count);

    // update the time of the last read from this directory stream
    readdir_session->last_readdir_rpc_timestamp = time(NULL);

    // if the stream was read to the end, close it
    if (*end_of_stream == 1) {
        int error_code = remove_readdir_session(client_authsysparams, directory_inode_number, active_readdir_sessions);
        if (error_code > 0) {
            fprintf(stderr,
                    "read_from_directory: failed removing the ReadDir session client %s:%d:%d and directory %s\n",
                    client_authsysparams->machinename, client_authsysparams->uid, client_authsysparams->gid,
                    directory_absolute_path);

            // clean up the directory entries allocated so far
            clean_up_directory_entries_list(directory_entries_list_head);

            pthread_mutex_unlock(&readdir_sessions_mutex);

            return 7;
        }
    }

    pthread_mutex_unlock(&readdir_sessions_mutex);

    return 0;
}