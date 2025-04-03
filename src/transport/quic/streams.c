#include "streams.h"

#include "pthread.h"
#include "stdlib.h"

/*
 * Creates a new QUIC stream inside the given QUIC connection and returns it.
 *
 * Returns NULL on failure.
 *
 * The user of this function takes the responsibility to deallocate the
 * received stream.
 */
Stream *create_new_stream(struct quic_conn_t *quic_connection) {
    Stream *stream = malloc(sizeof(Stream));
    if (stream == NULL) {
        return NULL;
    }

    uint64_t stream_id;
    int error_code = quic_stream_bidi_new(quic_connection, 0, true, &stream_id);
    if (error_code != 0) {
        fprintf(stderr, "create_new_stream: failed to create a new QUIC stream");

        free(stream);

        return NULL;
    }

    stream->id = stream_id;
    stream->stream_in_use = false;

    return stream;
}

/*
 * If the given list of auxilliary streams has a free stream in the pool,
 * returns that stream. If there is no such stream we create a new stream in
 * the given QUIC connection if the limit on the number of streams has not
 * already been reached.
 *
 * Locks the given mutex before accessing the list.
 *
 * Returns NULL on failure to allocate an auxilliary stream.
 */
Stream *get_available_auxilliary_stream(struct quic_conn_t *quic_connection, StreamsList **auxilliary_streams_list,
                                        pthread_mutex_t *list_lock) {
    if (auxilliary_streams_list == NULL) {
        return NULL;
    }

    pthread_mutex_lock(list_lock);

    StreamsList *head = *auxilliary_streams_list;
    int number_of_streams = 0;
    while (head != NULL) {
        if (head->stream == NULL) {
            fprintf(stderr, "get_available_stream: NULL stream encountered in the stream pool");
            return NULL;
        }

        if (!head->stream->stream_in_use) {
            head->stream->stream_in_use = true;

            pthread_mutex_unlock(list_lock);

            return head->stream;
        }

        number_of_streams++;
    }

    if (number_of_streams == MAX_STREAMS_PER_CONNECTION) {
        return NULL;
    }

    // create a new auxilliary stream
    Stream *new_auxilliary_stream = create_new_stream(quic_connection);
    if (new_auxilliary_stream == NULL) {
        fprintf(stderr, "get_available_stream: failed to create a new auxilliary stream");

        pthread_mutex_unlock(list_lock);

        return NULL;
    }
    new_auxilliary_stream->stream_in_use = true;

    StreamsList *new_entry = malloc(sizeof(StreamsList));
    if (new_entry == NULL) {
        free(new_auxilliary_stream);

        pthread_mutex_unlock(list_lock);

        return NULL;
    }
    new_entry->stream = new_auxilliary_stream;
    new_entry->next = *auxilliary_streams_list;
    *auxilliary_streams_list = new_entry;

    pthread_mutex_unlock(list_lock);

    return new_auxilliary_stream;
}

/*
 * Deallocates the given list of streams.
 */
void free_streams_list(StreamsList *head, pthread_mutex_t *list_lock) {
    pthread_mutex_lock(list_lock);

    StreamsList *curr = head;
    while (curr != NULL) {
        Stream *stream = curr->stream;
        StreamsList *next = curr->next;

        free(stream);
        free(curr);

        curr = next;
    }

    pthread_mutex_unlock(list_lock);
}