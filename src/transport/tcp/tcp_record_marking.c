#include "tcp_record_marking.h"

/*
* Sends the buffer of the given number of bytes to the given opened socket.
*
* Returns 0 on success and > 0 on failure.
*/
int send_bytes_tcp(int socket_fd, const uint8_t *source_buffer, size_t buffer_size) {
    size_t total_bytes_sent = 0;
    while (total_bytes_sent < buffer_size) {
        size_t bytes_sent = send(socket_fd, source_buffer + total_bytes_sent, buffer_size - total_bytes_sent, 0);
        if (bytes_sent < 0) {
            perror("send_bytes_tcp: failed to send from the buffer");
            return 1;
        }
        if(bytes_sent == 0) {  // to avoid infinite loops
            perror("send_bytes_tcp: sent zero bytes from the buffer");
            return 2;
        }

        total_bytes_sent += bytes_sent;
    }

    return 0;
}

/*
* Given an open socket and a buffer of Record Marking record data 'rm_record_data' of given
* size 'rm_record_data_size', sends the RM record data as a series of RM fragments to the
* specified socket.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rm_record_tcp(int socket_fd, const uint8_t *rm_record_data, size_t rm_record_data_size) {
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
        int error_code = send_bytes_tcp(socket_fd, header_buffer, sizeof(fragment_header));
        if(error_code > 0) {
            fprintf(stderr, "send_rm_record_tcp: failed to send the Record Marking fragment header\n");
            return 1;
        }
        
        // send the fragment data
        error_code = send_bytes_tcp(socket_fd, rm_record_data + rm_record_data_bytes_sent, fragment_size);
        if(error_code > 0) {
            fprintf(stderr, "send_rm_record_tcp: failed to send the Record Marking fragment data\n");
            return 2;
        }

        rm_record_data_bytes_sent += fragment_size;
    }

    return 0;
}

/*
* Given an opened socket, reads the given number of bytes from it and places them
* in the 'destination_buffer'.
*
* Returns 0 on success and > 0 on failure.
*/
int receive_bytes_tcp(int socket_fd, size_t bytes_to_read, uint8_t *destination_buffer) {
    size_t total_bytes_read = 0;
    while (total_bytes_read < bytes_to_read) {
        size_t bytes_left = bytes_to_read - total_bytes_read;
        size_t bytes_read = recv(socket_fd, destination_buffer + total_bytes_read, bytes_left, 0);
        if (bytes_read < 0) {
            perror("receive_bytes_tcp: failed to read from the socket");
            return 1;
        }
        if(bytes_read == 0) {  // to avoid infinite loops
            perror("receive_bytes_tcp: read zero bytes from the socket");
            return 2;
        }

        total_bytes_read += bytes_read;
    }

    return 0;
}

/*
* Given an open socket, reads a single Record Marking (RM) record (RFC 5531) from it.
* The data extracted from each RM fragment is concatenated and returned as result, and 
* the total number of bytes in the that result is placed in 'rm_record_data_size'.
*
* Returns NULL if reading the RM record was unsuccessful.
* 
* The user of this function takes the reponsibility to free the buffer returned as the result. 
*/
uint8_t *receive_rm_record_tcp(int socket_fd, size_t *rm_record_data_size) {
    uint8_t *rm_record_data_buffer = NULL;
    size_t rm_record_data_bytes_read = 0;
    int is_last_fragment = 0;

    while(!is_last_fragment) {
        // read the RM fragment header
        uint8_t fragment_header_buffer[RM_FRAGMENT_HEADER_SIZE];
        int error_code = receive_bytes_tcp(socket_fd, RM_FRAGMENT_HEADER_SIZE, fragment_header_buffer);
        if(error_code > 0) {
            fprintf(stderr, "receive_rm_record_tcp: failed to receive the Record Marking fragment header\n");

            free(rm_record_data_buffer);

            return NULL;
        }
        uint32_t fragment_header = 0;
        memcpy(&fragment_header, fragment_header_buffer, sizeof(fragment_header));

        // decode the fragment header
        fragment_header = ntohl(fragment_header);   // convert to host byte order
        is_last_fragment = (fragment_header & 0x80000000) != 0;   // if most significant bit is set, this is the last fragment
        size_t fragment_size = fragment_header & 0x7FFFFFFF;      // lower 31 bits specify fragment size in bytes

        // read fragment data
        rm_record_data_buffer = realloc(rm_record_data_buffer, rm_record_data_bytes_read + fragment_size);
        error_code = receive_bytes_tcp(socket_fd, fragment_size, rm_record_data_buffer + rm_record_data_bytes_read);
        if(error_code > 0) {
            fprintf(stderr, "receive_rm_record_tcp: failed to receive the Record Marking fragment data\n");

            free(rm_record_data_buffer);

            return NULL;
        }

        rm_record_data_bytes_read += fragment_size;
    }

    *rm_record_data_size = rm_record_data_bytes_read;

    return rm_record_data_buffer;
}