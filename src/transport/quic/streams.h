#ifndef streams__HEADER__INCLUDED
#define streams__HEADER__INCLUDED

#include "stdbool.h"
#include "stdint.h"
#include "tquic.h"

#define MAX_STREAMS_PER_CONNECTION 10

typedef struct Stream {
    uint64_t id;

    bool stream_in_use;
} Stream;

typedef struct StreamsList {
    Stream *stream;

    struct StreamsList *next;
} StreamsList;

Stream *create_new_stream(struct quic_conn_t *quic_connection);

Stream *get_available_auxilliary_stream(struct quic_conn_t *quic_connection, StreamsList **auxilliary_streams_list,
                                        pthread_mutex_t *list_lock);

void free_streams_list(StreamsList *head, pthread_mutex_t *list_lock);

#endif /* streams__HEADER__INCLUDED */