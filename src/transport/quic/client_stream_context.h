#ifndef client_stream_context__HEADER__INCLUDED
#define client_stream_context__HEADER__INCLUDED

#include "quic_record_marking.h"
#include "streams.h"

#include "pthread.h"

typedef struct QuicClientStreamContext {
    Stream *designated_stream;
    pthread_mutex_t lock;
    pthread_cond_t cond;

    // the RPC message to be sent by the client
    size_t call_rpc_msg_size;
    uint8_t *call_rpc_msg_buffer;

    // the RPC message being received as a Record Marking record
    RecordMarkingReceivingContext *rm_receiving_context;

    bool attempted_call_rpc_msg_send, call_rpc_msg_successfully_sent;
    bool reply_rpc_msg_successfully_received;
    bool finished;
} QuicClientStreamContext;

typedef struct QuicClientStreamContextsList {
    QuicClientStreamContext *stream_context;

    struct QuicClientStreamContextsList *next;
} QuicClientStreamContextsList;

QuicClientStreamContext *create_client_stream_context(uint8_t *rpc_msg_buffer, size_t rpc_msg_size);

int add_stream_context(QuicClientStreamContext *stream_context, QuicClientStreamContextsList **head,
                       pthread_mutex_t *list_lock);

QuicClientStreamContext *find_stream_context(QuicClientStreamContextsList *head, uint64_t stream_id,
                                             pthread_mutex_t *list_lock);

int remove_stream_context(QuicClientStreamContextsList **head, uint64_t stream_id, pthread_mutex_t *list_lock,
                          pthread_cond_t *list_cond);

void free_stream_contexts_list(QuicClientStreamContextsList *head, pthread_mutex_t *list_lock,
                               pthread_cond_t *list_cond);

#endif /* client_stream_context__HEADER__INCLUDED */