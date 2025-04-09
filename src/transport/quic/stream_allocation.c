#include "stream_allocation.h"

#include "quic_client.h"

/*
 * Pushes the given QUIC client stream context to the back of the
 * stream allocation queue.
 *
 * Returns 0 on success and > 0 on failure.
 *
 * The memory allocated in this function for the new queue entry should later
 * be freed in the 'pop_from_stream_allocation_queue' function.
 */
int push_to_stream_allocation_queue(QuicClientStreamContext *stream_context, StreamAllocationQueue **back,
                                    StreamAllocationQueue **front) {
    if (back == NULL) {
        return 1;
    }

    if (front == NULL) {
        return 2;
    }

    StreamAllocationQueue *new_entry = malloc(sizeof(StreamAllocationQueue));
    if (new_entry == NULL) {
        return 3;
    }

    new_entry->stream_context = stream_context;
    new_entry->prev = NULL;
    new_entry->next = *back;
    if (*back != NULL) {
        (*back)->prev = new_entry;
    }

    *back = new_entry;

    if (*front == NULL) {
        *front = new_entry;
    }

    return 0;
}

/*
 * Removes one QUIC client stream context from the front of the given stream
 * allocation queue. If the queue is empty, fails.
 *
 * Returns 0 on success and > 0 on failure.
 */
int pop_from_stream_allocation_queue(StreamAllocationQueue **back, StreamAllocationQueue **front) {
    if (front == NULL) {
        return 1;
    }

    if (*front == NULL) {
        return 2;
    }

    StreamAllocationQueue *first = *front;
    if (first->prev != NULL) {
        (first->prev)->next = NULL;
        *front = first->prev;
    } else {
        *front = NULL;
        *back = NULL;
    }

    free(first);

    return 0;
}

/*
 * Deallocates all stream allocation queue entries.
 */
void free_stream_allocation_queue(StreamAllocationQueue *back) {
    StreamAllocationQueue *curr = back;
    while (curr != NULL) {
        StreamAllocationQueue *next = curr->next;
        free(curr);

        curr = next;
    }
}

/*
 * Allocates a QUIC stream for the given client stream context in the given QUIC client.
 *
 * Returns 0 on success and > 0 on failure.
 */
int allocate_stream(QuicClient *client, QuicClientStreamContext *stream_context) {
    pthread_mutex_lock(&stream_context->stream_allocator_lock);

    Stream *quic_stream = NULL;
    if (stream_context->use_auxiliary_stream) {
        Stream *allocated_auxiliary_stream = get_available_auxiliary_stream(
            client->quic_connection, &client->auxiliary_streams_list_head, &client->auxiliary_streams_list_lock);
        if (allocated_auxiliary_stream == NULL) {
            fprintf(stderr, "allocate_stream: failed to allocate an auxiliary stream\n");

            stream_context->successfully_allocated_stream = false;
            stream_context->stream_allocation_finished = true;
            pthread_cond_signal(&stream_context->stream_allocator_condition_variable);

            return 1;
        }

        quic_stream_wantwrite(client->quic_connection, allocated_auxiliary_stream->id, true);

        quic_stream = allocated_auxiliary_stream;
    } else {
        if (client->main_stream == NULL) {
            fprintf(stderr, "allocate_stream: failed to create the main stream\n");

            stream_context->successfully_allocated_stream = false;
            stream_context->stream_allocation_finished = true;
            pthread_cond_signal(&stream_context->stream_allocator_condition_variable);

            return 2;
        }

        quic_stream_wantwrite(client->quic_connection, client->main_stream->id, true);

        quic_stream = client->main_stream;
    }

    stream_context->allocated_stream = quic_stream;
    stream_context->successfully_allocated_stream = true;
    stream_context->stream_allocation_finished = true;

    pthread_mutex_unlock(&stream_context->stream_allocator_lock);
    pthread_cond_signal(&stream_context->stream_allocator_condition_variable);

    return 0;
}

/*
 * Processes all stream allocation queue entries, allocates streams for each QUIC
 * stream context in it, and removes all queue entries.
 *
 * Executes atomically by acquiring the QUIC client's stream allocation mutex.
 */
void async_stream_allocator_callback(EV_P_ ev_async *w, int revents) {
    QuicClient *client = w->data;

    pthread_mutex_lock(&client->stream_allocation_queue_lock);

    StreamAllocationQueue *curr = client->stream_allocation_queue_front;
    while (curr != NULL) {
        int error_code = allocate_stream(client, curr->stream_context);
        if (error_code > 0) {
            fprintf(stderr, "async_stream_allocator_callback: failed to allocate stream\n");
        }

        StreamAllocationQueue *prev = curr->prev;
        error_code = pop_from_stream_allocation_queue(&client->stream_allocation_queue_back,
                                                      &client->stream_allocation_queue_front);
        if (error_code > 0) {
            fprintf(stderr, "async_stream_allocator_callback: failed to pop stram allocation queue entry\n");

            pthread_mutex_unlock(&client->stream_allocation_queue_lock);

            return;
        }
        curr = prev;
    }

    pthread_mutex_unlock(&client->stream_allocation_queue_lock);
}