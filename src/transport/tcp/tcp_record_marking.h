#ifndef tcp_record_marking__header__INCLUDED
#define tcp_record_marking__header__INCLUDED

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "src/transport/transport_common.h"

int send_rm_record_tcp(int socket_fd, const uint8_t *rm_record_data, size_t rm_record_data_size);

uint8_t *receive_rm_record_tcp(int socket_fd, size_t *rpc_message_size);

#endif /* tcp_record_marking__header__INCLUDED */