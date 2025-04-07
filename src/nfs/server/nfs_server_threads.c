#include "nfs_server_threads.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // read(), write(), close()

pthread_mutex_t nfs_server_threads_list_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Frees the given NfsServerThreadsList entry.
 * In case of a TCP transport connection, closes and frees the rpc_client_socket_fd
 * inside the given nfs server threads list entry.
 *
 * Does nothing if the given NfsServerThreadsList is null.
 */
void clean_up_nfs_server_threads_list_entry(NfsServerThreadsList *server_threads_list_entry) {
    if (server_threads_list_entry == NULL) {
        return;
    }

    switch (server_threads_list_entry->transport_protocol) {
    case TRANSPORT_PROTOCOL_TCP:
        if (server_threads_list_entry->transport_connection.tcp_client->tcp_rpc_client_socket_fd == NULL) {
            break;
        }

        close(*server_threads_list_entry->transport_connection.tcp_client->tcp_rpc_client_socket_fd);
        free(server_threads_list_entry->transport_connection.tcp_client->tcp_rpc_client_socket_fd);

        break;
    case TRANSPORT_PROTOCOL_QUIC:
        // QUIC
    default:
    }

    free(server_threads_list_entry);
}

/*
 * Creates a new entry for the given created NFS client-handling thread with the given transport connection
 * parameter, and adds it to the head of the given inode cache 'head'.
 *
 * The user of this function takes the responsiblity to free deallocate the created entry.
 * This should be done when this thread terminates, or on server shutdown when the entire list of
 * client-handling threads is deallocated.
 *
 * This function is thread-safe - it waits on a common mutex before modiyfing the nfs server threads list.
 *
 * Returns 0 on success and > 0 on failure.
 */
int add_server_thread(pthread_t server_thread, TransportProtocol transport_protocol,
                      TransportConnection transport_connection, NfsServerThreadsList **head) {
    NfsServerThreadsList *new_entry = malloc(sizeof(NfsServerThreadsList));
    if (new_entry == NULL) {
        return 1;
    }

    new_entry->server_thread = server_thread;
    new_entry->transport_protocol = transport_protocol;
    new_entry->transport_connection = transport_connection;

    new_entry->next = *head;

    pthread_mutex_lock(&nfs_server_threads_list_mutex);
    *head = new_entry;
    pthread_mutex_unlock(&nfs_server_threads_list_mutex);

    return 0;
}

/*
 * Removes an entry for the given server thread from the given list of server threads,
 * and cleans up that removed NfsServerThreadsList entry.
 *
 * This function is thread-safe - it waits on a common mutex before modiyfing the nfs server threads list.
 *
 * Returns 0 on successful removal of the entry, 1 if the corresponding entry
 * could not be found, and > 1 on failure otherwise.
 */
int remove_server_thread(pthread_t server_thread, NfsServerThreadsList **head) {
    if (head == NULL) {
        return 2;
    }

    pthread_mutex_lock(&nfs_server_threads_list_mutex);

    // the entry we want to remove is the first in the list
    if ((*head)->server_thread == server_thread) {
        NfsServerThreadsList *new_head = (*head)->next;

        clean_up_nfs_server_threads_list_entry(*head);

        *head = new_head;

        return 0;
    }

    // the entry we want to remove is somewhere in the middle of the list
    NfsServerThreadsList *curr_entry = (*head)->next, *prev_mapping = *head;
    while (curr_entry != NULL) {
        if (curr_entry->server_thread == server_thread) {
            prev_mapping->next = curr_entry->next;

            clean_up_nfs_server_threads_list_entry(curr_entry);

            pthread_mutex_unlock(&nfs_server_threads_list_mutex);

            return 0;
        }

        prev_mapping = curr_entry;
        curr_entry = curr_entry->next;
    }

    pthread_mutex_unlock(&nfs_server_threads_list_mutex);

    return 1;
}

/*
 * Cleans up all entries in the given NfsServerThreadsList.
 *
 * This function is thread-safe - it waits on a common mutex before modiyfing the nfs server threads list.
 *
 * This function must only be called from the main NFS process (not from a client-handling server thread) when
 * it wants to terminate all server threads.
 */
void clean_up_nfs_server_threads_list(NfsServerThreadsList *nfs_server_threads_list) {
    pthread_mutex_lock(&nfs_server_threads_list_mutex);

    NfsServerThreadsList *curr_entry = nfs_server_threads_list;
    while (curr_entry != NULL) {
        NfsServerThreadsList *next = curr_entry->next;

        // terminate the server thread
        pthread_cancel(curr_entry->server_thread);
        pthread_join(curr_entry->server_thread, NULL);
        fprintf(stdout, "Killed NFS server thread %ld\n", curr_entry->server_thread);

        clean_up_nfs_server_threads_list_entry(curr_entry);

        curr_entry = next;
    }

    pthread_mutex_unlock(&nfs_server_threads_list_mutex);
}