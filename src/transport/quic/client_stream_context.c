#include "client_stream_context.h"

/*
 * Creates an empty QUIC client stream context for the given QUIC client with the given RPC
 * call message, and returns it.
 *
 * Returns NULL on failure.
 *
 * The user of this function takes the responsibility to deallocated the created stream context.
 */
QuicClientStreamContext *create_client_stream_context(QuicClient *quic_client, uint8_t *rpc_msg_buffer,
                                                      size_t rpc_msg_size, bool use_auxiliary_stream) {
    QuicClientStreamContext *stream_context = malloc(sizeof(QuicClientStreamContext));
    if (stream_context == NULL) {
        return NULL;
    }

    stream_context->quic_client = quic_client;

    stream_context->use_auxiliary_stream = use_auxiliary_stream;
    stream_context->allocated_stream = NULL;
    stream_context->successfully_allocated_stream = stream_context->stream_allocation_finished = false;
    pthread_mutex_init(&stream_context->stream_allocator_lock, NULL);
    pthread_cond_init(&stream_context->stream_allocator_condition_variable, NULL);

    stream_context->rm_receiving_context = NULL;

    stream_context->attempted_call_rpc_msg_send = stream_context->call_rpc_msg_successfully_sent = false;
    stream_context->reply_rpc_msg_successfully_received = false;
    stream_context->finished = false;
    pthread_mutex_init(&stream_context->reply_lock, NULL);
    pthread_cond_init(&stream_context->reply_cond, NULL);

    stream_context->call_rpc_msg_size = rpc_msg_size;
    stream_context->call_rpc_msg_buffer = rpc_msg_buffer;

    return stream_context;
}

/*
 * Deallocates the given stream context.
 *
 * Does nothing if the given stream context is NULL.
 */
void free_stream_context(QuicClientStreamContext *stream_context) {
    if (stream_context == NULL) {
        return;
    }

    pthread_mutex_destroy(&stream_context->stream_allocator_lock);
    pthread_cond_destroy(&stream_context->stream_allocator_condition_variable);

    pthread_mutex_destroy(&stream_context->reply_lock);
    pthread_cond_destroy(&stream_context->reply_cond);

    if (stream_context->call_rpc_msg_buffer != NULL) {
        free(stream_context->call_rpc_msg_buffer);
    }
    free_rm_receiving_context(stream_context->rm_receiving_context);

    free(stream_context);
}

/*
 * Adds the given stream context to the given list of stream contexts.
 *
 * Returns > 0 on failure and 0 on success.
 *
 * The user of this function takes the responsibility to free the added list entry.
 */
int add_stream_context(QuicClientStreamContext *stream_context, QuicClientStreamContextsList **head) {
    if (stream_context == NULL) {
        return 1;
    }

    if (head == NULL) {
        return 2;
    }

    QuicClientStreamContextsList *new_head = malloc(sizeof(QuicClientStreamContextsList));
    if (new_head == NULL) {
        return 3;
    }

    new_head->stream_context = stream_context;
    new_head->next = *head;
    *head = new_head;

    return 0;
}

/*
 * Searches the given list of stream contexts for the client context of the stream with the given ID.
 *
 * Returns the found stream context or NULL if the context is not present in the list.
 */
QuicClientStreamContext *find_stream_context(QuicClientStreamContextsList *head, uint64_t stream_id) {
    QuicClientStreamContextsList *curr = head;
    while (curr != NULL) {
        QuicClientStreamContext *stream_context = curr->stream_context;
        if (stream_context == NULL) {
            fprintf(stderr, "find_stream_context: NULL stream context encountered\n");
            return NULL;
        }

        if (stream_context->allocated_stream == NULL) {
            fprintf(stderr, "find_stream_context: stream context with no allocated stream encountered\n");
            return NULL;
        }

        if (stream_context->allocated_stream->id == stream_id) {
            return stream_context;
        }

        curr = curr->next;
    }

    return NULL;
}

/*
 * Removes the entry with the given stream ID from the given list of stream contexts, if it exists,
 * and deallocates that list entry (the QuicClientStreamContextsList) and the stream context itself
 * (the QuicClientStreamContext).
 *
 * Returns 0 on success and > 0 on failure.
 */
int remove_stream_context(QuicClientStreamContextsList **head, uint64_t stream_id) {
    if (head == NULL) {
        return 1;
    }

    if (*head == NULL) {
        // empty list of stream contexts
        return 0;
    }

    QuicClientStreamContextsList *first_entry = *head;
    QuicClientStreamContext *first_stream_context = first_entry->stream_context;
    if (first_stream_context == NULL) {
        fprintf(stderr, "remove_stream_context: NULL stream context encountered\n");
        return 2;
    }
    if (first_stream_context->allocated_stream == NULL) {
        fprintf(stderr, "remove_stream_context: stream context with no allocated stream encountered\n");
        return 3;
    }
    if (first_stream_context->allocated_stream->id == stream_id) {
        *head = (*head)->next;

        free_stream_context(first_stream_context);
        free(first_entry);

        return 0;
    }

    QuicClientStreamContextsList *curr = *head, *prev = NULL;
    while (curr != NULL) {
        QuicClientStreamContext *stream_context = curr->stream_context;
        if (stream_context == NULL) {
            fprintf(stderr, "find_stream_context: NULL stream context encountered\n");
            return 4;
        }
        if (stream_context->allocated_stream == NULL) {
            fprintf(stderr, "find_stream_context: stream context with no allocated stream encountered\n");
            return 5;
        }

        if (stream_context->allocated_stream->id == stream_id) {
            prev->next = curr->next;

            free_stream_context(curr->stream_context);
            free(curr);

            return 0;
        }

        prev = curr;
        curr = curr->next;
    }

    return 0;
}

/*
 * Deallocates the given list of stream contexts and the entries inside it.
 */
void free_stream_contexts_list(QuicClientStreamContextsList *head, pthread_mutex_t *list_lock,
                               pthread_cond_t *list_cond) {
    pthread_mutex_lock(list_lock);

    // wait for all stream contexts to remove themselves from the list (when their RPCs are done)
    while (head != NULL) {
        pthread_cond_wait(list_cond, list_lock);
    }

    pthread_mutex_unlock(list_lock);
}