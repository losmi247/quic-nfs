#ifndef client_common_rpc__header__INCLUDED
#define client_common_rpc__header__INCLUDED

/*
* Functions implemented in client_common_rpc.c file.
*/

Google__Protobuf__Any *invoke_rpc(const char *server_ipv4_addr, uint16_t server_port, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters);

#endif /* client_common_rpc__header__INCLUDED */