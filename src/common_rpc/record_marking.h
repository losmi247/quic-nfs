#ifndef record_marking__header__INCLUDED
#define record_marking__header__INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#define RM_FRAGMENT_HEADER_SIZE 4             // RPC Record Marking fragment header size in bytes
#define RM_MAX_FRAGMENT_DATA_SIZE 0x7FFFFFFF  // max size in bytes of the data in a Record Marking fragment

uint8_t *receive_rm_record(int socket_fd, size_t *rpc_message_size);
int send_rm_record(int socket_fd, const uint8_t *rm_record_data, size_t rm_record_data_size);

#endif /* record_marking__header__INCLUDED */