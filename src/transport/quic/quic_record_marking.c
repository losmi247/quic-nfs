#include "quic_record_marking.h"

#include "src/transport/transport_common.h"

/*
* Sends the buffer of the given number of bytes to the stream with the given ID in the
* given QUIC connection.
*
* Returns 0 on success and > 0 on failure.
*/
int send_bytes_quic(struct quic_conn_t *conn, uint64_t stream_id, const uint8_t *source_buffer, size_t buffer_size) {
    size_t total_bytes_sent = 0;
    while (total_bytes_sent < buffer_size) {
        size_t bytes_sent = quic_stream_write(conn, stream_id, source_buffer + total_bytes_sent, buffer_size - total_bytes_sent, false);
        if (bytes_sent < 0) {
            fprintf(stderr, "send_bytes_quic: failed to send from the buffer, error code %ld\n", bytes_sent);
            return 1;
        }
        if(bytes_sent == 0) {  // to avoid infinite loops
            fprintf(stderr, "send_bytes_quic: sent zero bytes from the buffer\n");
            return 2;
        }

        total_bytes_sent += bytes_sent;
    }

    return 0;
}

/*
* Given a QUIC connection and an ID of a stream inside it, and a buffer of Record Marking record data
* 'rm_record_data' of given size 'rm_record_data_size', sends the RM record data as a series of RM fragments to the
* specified QUIC stream.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rm_record_quic(struct quic_conn_t *conn, uint64_t stream_id, const uint8_t *rm_record_data, size_t rm_record_data_size) {
    size_t rm_record_data_bytes_sent = 0;

    while(rm_record_data_bytes_sent < rm_record_data_size) {
        size_t bytes_left = rm_record_data_size - rm_record_data_bytes_sent;
        size_t fragment_size = (bytes_left < RM_MAX_FRAGMENT_DATA_SIZE) ? bytes_left : RM_MAX_FRAGMENT_DATA_SIZE;

        // create the RM fragment header
        uint32_t fragment_header = (bytes_left == fragment_size ? 0x80000000 : 0) | fragment_size;  // set the MSB if this is the last fragment
        fragment_header = htonl(fragment_header);   // convert to network byte order

        // send the fragment header
        uint8_t header_buffer[sizeof(fragment_header)];
        memcpy(header_buffer, &fragment_header, sizeof(fragment_header));
        int error_code = send_bytes_quic(conn, stream_id, header_buffer, sizeof(fragment_header));
        if(error_code > 0) {
            fprintf(stderr, "send_rm_record_quic: failed to send the Record Marking fragment header\n");
            return 1;
        }
        
        // send the fragment data
        error_code = send_bytes_quic(conn, stream_id, rm_record_data + rm_record_data_bytes_sent, fragment_size);
        if(error_code > 0) {
            fprintf(stderr, "send_rm_record_quic: failed to send the Record Marking fragment data\n");
            return 2;
        }

        rm_record_data_bytes_sent += fragment_size;
    }

    return 0;
}

/*
* Creates a fresh RM receiving context (with buffer size 0, and buffer being NULL) for the given stream in the 
* given QUIC connection.
*
* Returns NULL on failure.
*
* The user of this function takes the responsibility to free the created RM receiving context using the
* 'free_rm_receiving_context' function.
*/
RecordMarkingReceivingContext *create_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id) {
    RecordMarkingReceivingContext *rm_receiving_context = malloc(sizeof(RecordMarkingReceivingContext));
    if(rm_receiving_context == NULL) {
        fprintf(stderr, "create_rm_receiving_context: failed to allocate memory\n");
        return NULL;
    }

    rm_receiving_context->quic_connection = quic_connection;
    rm_receiving_context->stream_id = stream_id;

    rm_receiving_context->accumulated_payloads_size = 0;
    rm_receiving_context->accumulated_payloads = NULL;

    rm_receiving_context->current_rm_fragment_num_received_header_bytes = 0;
    rm_receiving_context->current_rm_fragment_received_header_bytes = NULL;

    rm_receiving_context->is_last_fragment = false;

    rm_receiving_context->current_rm_fragment_expected_size = 0;
    rm_receiving_context->current_rm_fragment_num_received_payload_bytes = 0;
    rm_receiving_context->current_rm_fragment_received_payload_bytes = NULL;

    rm_receiving_context->record_fully_received = false;

    return rm_receiving_context;
}

/*
* Resets the state in the given RM receiving context to prepare it to receive
* the next RM fragment.
*
* Does nothing if the given RM receiving context is NULL.
*/
void reset_rm_receiving_context_for_next_rm_fragment(RecordMarkingReceivingContext *rm_receiving_context) {
    if(rm_receiving_context == NULL) {
        return;
    }

    rm_receiving_context->current_rm_fragment_num_received_header_bytes = 0;
    rm_receiving_context->current_rm_fragment_received_header_bytes = NULL;

    rm_receiving_context->is_last_fragment = false;

    rm_receiving_context->current_rm_fragment_expected_size = 0;
    rm_receiving_context->current_rm_fragment_num_received_payload_bytes = 0;
    rm_receiving_context->current_rm_fragment_received_payload_bytes = NULL;

    rm_receiving_context->record_fully_received = false;
}

/*
* Deallocates all heap allocated memory in the given RM receiving context, including
* the context itself.
*
* Does nothing if the given RM receiving context is NULL.
*/
void free_rm_receiving_context(RecordMarkingReceivingContext *rm_receiving_context) {
    if(rm_receiving_context == NULL) {
        return;
    }

    free(rm_receiving_context->accumulated_payloads);

    free(rm_receiving_context);
}

/*
* Creates a fresh RM receiving context (with buffer size 0) for the given stream in the 
* given QUIC connection, and adds it to the head of the given list of RM receiving contexts.
*
* Returns 0 on success and > 0 on failure.
*
* The user of this function takes the responsibility to free the RM receiving contexts added
* to the list using 'remove_rm_receiving_context'.
*/
int add_new_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id, RecordMarkingReceivingContextsList **rm_receiving_contexts) {
    if(rm_receiving_contexts == NULL) {
        fprintf(stderr, "add_new_rm_receiving_context: list of RM receiving contexts is NULL\n");
        return 1;
    }

    RecordMarkingReceivingContext *new_rm_receiving_context = create_rm_receiving_context(quic_connection, stream_id);
    if(new_rm_receiving_context == NULL) {
        return 2;
    }

    RecordMarkingReceivingContextsList *new_head = malloc(sizeof(RecordMarkingReceivingContextsList));
    if(new_head == NULL) {
        fprintf(stderr, "add_new_rm_receiving_context: failed to allocate memory\n");
        free_rm_receiving_context(new_rm_receiving_context);
        return 3;
    }
    new_head->rm_receiving_context = new_rm_receiving_context;
    new_head->next = *rm_receiving_contexts;

    *rm_receiving_contexts = new_head;

    return 0;
}

/*
* Checks if the RM receiving context for the given stream in the given QUIC connection is present inside the given list of RM receiving contexts.
*
* Returns 0 if the RM receieving context for this stream was found in the list, and 1 otherwise.
*/
int find_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id, RecordMarkingReceivingContextsList *rm_receiving_contexts) {
    RecordMarkingReceivingContextsList *curr = rm_receiving_contexts;
    while(curr != NULL) {
        RecordMarkingReceivingContext *context = curr->rm_receiving_context;
        if(context == NULL) {
            fprintf(stderr, "find_rm_receiving_context: encountered a NULL RM receiving context in the list\n");

            curr = curr->next;

            continue;
        }
        if(context->quic_connection == quic_connection && context->stream_id == stream_id) {
            return 0;
        }

        curr = curr->next;
    }

    return 1;
}

/*
* Finds the RM receiving context for the given stream in the given QUIC connection inside the given list of RM receiving contexts,
* and returns that RM receiving context.
*
* Returns NULL on failure (e.g. context for this stream not found).
*/
RecordMarkingReceivingContext *get_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id, RecordMarkingReceivingContextsList *rm_receiving_contexts) {
    RecordMarkingReceivingContextsList *curr = rm_receiving_contexts;
    while(curr != NULL) {
        RecordMarkingReceivingContext *context = curr->rm_receiving_context;
        if(context == NULL) {
            fprintf(stderr, "get_rm_receiving_context: encountered a NULL RM receiving context in the list\n");
            return NULL;
        }
        if(context->quic_connection == quic_connection && context->stream_id == stream_id) {
            return context;
        }

        curr = curr->next;
    }

    return NULL;
}

/*
* Locates the RM receiving context for the given stream in the given QUIC connection inside the given RM receiving contexts list,
* removes it from that list, and frees that RM receiving context.
*
* Returns 0 on success and > 0 on failure (e.g. context for this stream not found).
*/
int remove_rm_receiving_context(struct quic_conn_t *quic_connection, uint64_t stream_id, RecordMarkingReceivingContextsList **rm_receiving_contexts) {
    if(rm_receiving_contexts == NULL) {
        fprintf(stderr, "remove_rm_receiving_context: RM receiving context not found\n");
        return 1;
    }

    // the entry we want to remove is the first in the list
    RecordMarkingReceivingContext *first_context = (*rm_receiving_contexts)->rm_receiving_context;
    if(first_context == NULL) {
        fprintf(stderr, "remove_rm_receiving_context: encountered a NULL RM receiving context in the list\n");
        return 2;
    }
    if(first_context->quic_connection == quic_connection && first_context->stream_id == stream_id) {
        RecordMarkingReceivingContextsList *new_head = (*rm_receiving_contexts)->next;

        free_rm_receiving_context(first_context);
        free(*rm_receiving_contexts);

        *rm_receiving_contexts = new_head;

        return 0;
    }

    // the entry we want to remove is somewhere in the middle of the list
    RecordMarkingReceivingContextsList *curr = (*rm_receiving_contexts)->next, *prev = *rm_receiving_contexts;
    while(curr != NULL) {
        RecordMarkingReceivingContext *context = curr->rm_receiving_context;
        if(context == NULL) {
            fprintf(stderr, "remove_rm_receiving_context: encountered a NULL RM receiving context in the list\n");
            return 3;
        }
        if(context->quic_connection == quic_connection && context->stream_id == stream_id) {
            prev->next = curr->next;

            free_rm_receiving_context(context);
            free(curr);

            return 0;
        }

        prev = curr;
        curr = curr->next;
    }

    fprintf(stderr, "remove_rm_receiving_context: RM receiving context not found\n");
    return 4;
}

/*
* Deallocates all RM receiving records in the given list.
*/
void clean_up_rm_receiving_contexts_list(RecordMarkingReceivingContextsList *rm_receiving_contexts) {
    while(rm_receiving_contexts != NULL) {
        RecordMarkingReceivingContextsList *next = rm_receiving_contexts->next;

        free_rm_receiving_context(rm_receiving_contexts->rm_receiving_context);
        free(rm_receiving_contexts);

        rm_receiving_contexts = next;
    }
}

/*
* Given a RM receiving context, reads all available data from this context's stream, updating
* the state of the given RM receiving context to contain the newly received data.
*
* When it receives a complete Record Marking record, if there is more data to be read from this stream
* then this function fails.
*
* Returns 0 on success and > 0 on failure.
*/
int receive_available_bytes_quic(RecordMarkingReceivingContext *rm_receiving_context) {
    if(rm_receiving_context == NULL) {
        return 1;
    }

    // read all newly available data
    uint8_t read_buffer[READ_BUF_SIZE];
    bool fin = false;
    while(1) {
        ssize_t bytes_read = quic_stream_read(rm_receiving_context->quic_connection, rm_receiving_context->stream_id, read_buffer, sizeof(read_buffer), &fin);
        if(bytes_read < 0) {
            // all currently available data has been read from this stream
            break;
        }
        if(bytes_read == 0) {   // to prevent infinite loops
            fprintf(stderr, "receive_available_bytes_quic: read 0 bytes from stream %ld\n", rm_receiving_context->stream_id);
            return 2;
        }

        size_t bytes_processed = 0;
        while(bytes_processed < bytes_read) {
            // if not fully received, read the RM fragment header
            if(rm_receiving_context->current_rm_fragment_num_received_header_bytes < RM_FRAGMENT_HEADER_SIZE) {
                if(rm_receiving_context->current_rm_fragment_received_header_bytes == NULL) {
                    rm_receiving_context->current_rm_fragment_received_header_bytes = malloc(sizeof(uint8_t) * RM_FRAGMENT_HEADER_SIZE);
                }
                if(rm_receiving_context->current_rm_fragment_received_header_bytes == NULL) {
                    fprintf(stderr, "receive_available_bytes_quic: failed to allocate memory\n");
                    return 3;
                }

                size_t bytes_to_process = bytes_read - bytes_processed;
                size_t header_bytes_left = RM_FRAGMENT_HEADER_SIZE - rm_receiving_context->current_rm_fragment_num_received_header_bytes;
                size_t header_bytes_to_process = header_bytes_left < bytes_to_process ? header_bytes_left : bytes_to_process;
                memcpy(rm_receiving_context->current_rm_fragment_received_header_bytes + rm_receiving_context->current_rm_fragment_num_received_header_bytes, read_buffer + bytes_processed, header_bytes_to_process);
            
                rm_receiving_context->current_rm_fragment_num_received_header_bytes += header_bytes_to_process;
                bytes_processed += header_bytes_to_process;

                // parse the fragment header if fully received
                if(rm_receiving_context->current_rm_fragment_num_received_header_bytes == RM_FRAGMENT_HEADER_SIZE) {
                    uint32_t fragment_header = 0;
                    memcpy(&fragment_header, rm_receiving_context->current_rm_fragment_received_header_bytes, sizeof(fragment_header));

                    // decode the fragment header
                    fragment_header = ntohl(fragment_header);                                                   // convert to host byte order
                    rm_receiving_context->is_last_fragment = (fragment_header & 0x80000000) != 0;               // if most significant bit is set, this is the last fragment
                    rm_receiving_context->current_rm_fragment_expected_size = fragment_header & 0x7FFFFFFF;     // lower 31 bits specify fragment size in bytes
                }

                continue;
            }

            // if not fully received, read the RM fragment payload
            if(rm_receiving_context->current_rm_fragment_num_received_payload_bytes < rm_receiving_context->current_rm_fragment_expected_size) {
                if(rm_receiving_context->current_rm_fragment_received_payload_bytes == NULL) {
                    rm_receiving_context->current_rm_fragment_received_payload_bytes = malloc(sizeof(uint8_t) * rm_receiving_context->current_rm_fragment_expected_size);
                }
                if(rm_receiving_context->current_rm_fragment_received_header_bytes == NULL) {
                    fprintf(stderr, "receive_available_bytes_quic: failed to allocate memory\n");
                    return 3;
                }

                size_t bytes_to_process = bytes_read - bytes_processed;
                size_t payload_bytes_left = rm_receiving_context->current_rm_fragment_expected_size - rm_receiving_context->current_rm_fragment_num_received_payload_bytes;
                size_t payload_bytes_to_process = payload_bytes_left < bytes_to_process ? payload_bytes_left : bytes_to_process;
                memcpy(rm_receiving_context->current_rm_fragment_received_payload_bytes + rm_receiving_context->current_rm_fragment_num_received_payload_bytes, read_buffer + bytes_processed, payload_bytes_to_process);

                rm_receiving_context->current_rm_fragment_num_received_payload_bytes += payload_bytes_to_process;
                bytes_processed += payload_bytes_to_process;
            }

            // the RM fragment was fully received
            if(rm_receiving_context->current_rm_fragment_num_received_payload_bytes == rm_receiving_context->current_rm_fragment_expected_size) {
                // allocate more memory for storing all RM fragment payloads
                if(rm_receiving_context->accumulated_payloads == NULL) {
                    rm_receiving_context->accumulated_payloads = malloc(sizeof(uint8_t) * rm_receiving_context->current_rm_fragment_num_received_payload_bytes);
                }
                else {
                    rm_receiving_context->accumulated_payloads = realloc(rm_receiving_context->accumulated_payloads, sizeof(uint8_t) * (rm_receiving_context->accumulated_payloads_size + rm_receiving_context->current_rm_fragment_num_received_payload_bytes));
                }
                if(rm_receiving_context->accumulated_payloads == NULL) {
                    fprintf(stderr, "receive_available_bytes_quic: failed to allocate memory\n");
                    return 3;
                }

                // save the payload
                memcpy(rm_receiving_context->accumulated_payloads + rm_receiving_context->accumulated_payloads_size, rm_receiving_context->current_rm_fragment_received_payload_bytes, rm_receiving_context->current_rm_fragment_num_received_payload_bytes);
                rm_receiving_context->accumulated_payloads_size += rm_receiving_context->current_rm_fragment_num_received_payload_bytes;

                // free the buffers used for RM fragment header and payload
                free(rm_receiving_context->current_rm_fragment_received_header_bytes);
                free(rm_receiving_context->current_rm_fragment_received_payload_bytes);

                // this was the last fragment of the RM record
                if(rm_receiving_context->is_last_fragment) {
                    rm_receiving_context->record_fully_received = true;
                    break;
                }

                // reset the context for receiving the next RM fragment
                reset_rm_receiving_context_for_next_rm_fragment(rm_receiving_context);
            }
        }

        if(rm_receiving_context->record_fully_received) {
            if(bytes_processed < bytes_read) {
                fprintf(stderr, "receive_available_bytes_quic: received more data following the RPC on stream %ld\n", rm_receiving_context->stream_id);
                return 4;
            }

            break;
        }
    }

    return 0;
}