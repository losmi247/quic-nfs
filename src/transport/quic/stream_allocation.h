#ifndef stream_allocation__HEADER__INCLUDED
#define stream_allocation__HEADER__INCLUDED

#include "client_stream_context.h"

typedef struct StreamAllocationQueue {
    QuicClientStreamContext *stream_context;
    struct StreamAllocationQueue *next;
    struct StreamAllocationQueue *prev;
} StreamAllocationQueue;

int push_to_stream_allocation_queue(QuicClientStreamContext *stream_context, StreamAllocationQueue **back,
                                    StreamAllocationQueue **front);

void free_stream_allocation_queue(StreamAllocationQueue *back);

void async_stream_allocator_callback(EV_P_ ev_async *w, int revents);

#endif