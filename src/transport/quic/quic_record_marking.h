#ifndef quic_record_marking__header__INCLUDED
#define quic_record_marking__header__INCLUDED

#include <errno.h>
#include <ev.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "tquic.h"

#define READ_BUF_SIZE 10000

/*
Sending Record Marking records
*/

int send_rm_record_quic(struct quic_conn_t *conn, uint64_t stream_id, const uint8_t *rm_record_data,
                        size_t rm_record_data_size);

/*
 * Receiving Record Marking records
 */

/*
 * RM Receiving Context
 */

typedef struct RecordMarkingReceivingContext {
    // the QUIC connection and the stream where this RM record is being received
    struct quic_conn_t *quic_connection;
    uint64_t stream_id;

    // concatenated payloads from the RM fragments that have been fully received so far
    size_t accumulated_payloads_size;
    uint8_t *accumulated_payloads;

    // bytes received so far in the header of the RM fragment currently being received
    size_t current_rm_fragment_num_received_header_bytes;
    uint8_t *current_rm_fragment_received_header_bytes;

    // is the last fragment currently being received
    bool is_last_fragment;

    // bytes received so far in the payload of the RM fragment currently being received
    size_t current_rm_fragment_expected_size;
    size_t current_rm_fragment_num_received_payload_bytes;
    uint8_t *current_rm_fragment_received_payload_bytes;

    bool record_fully_received;
} RecordMarkingReceivingContext;

RecordMarkingReceivingContext *create_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id);

void free_rm_receiving_context(RecordMarkingReceivingContext *rm_receiving_context);

/*
 * Lists of RM Receiving Contexts
 */

typedef struct RecordMarkingReceivingContextsList {
    RecordMarkingReceivingContext *rm_receiving_context;

    struct RecordMarkingReceivingContextsList *next;
} RecordMarkingReceivingContextsList;

int add_new_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id,
                                 RecordMarkingReceivingContextsList **rm_receiving_contexts);

int find_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id,
                              RecordMarkingReceivingContextsList *rm_receiving_contexts);

RecordMarkingReceivingContext *get_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id,
                                                        RecordMarkingReceivingContextsList *rm_receiving_contexts);

int remove_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id,
                                RecordMarkingReceivingContextsList **rm_receiving_contexts);

void clean_up_rm_receiving_contexts_list(RecordMarkingReceivingContextsList *rm_receiving_contexts);

int receive_available_bytes_quic(RecordMarkingReceivingContext *rm_receiving_context);

#endif /* quic_record_marking__header__INCLUDED */