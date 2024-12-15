#ifndef nfs_client__header__INCLUDED
#define nfs_client__header__INCLUDED

#include "src/serialization/nfs/nfs.pb-c.h"

/*
* Every procedure returns 0 on successful execution, and in that cases places non-NULL procedure results in the last argument that is passed to it.
* In case of unsuccessful execution, the procedure returns an error code > 0, from client_common_rpc.c.
*/

int nfs_procedure_0_do_nothing(const char *server_ipv4_addr, uint16_t server_port);

int nfs_procedure_1_get_file_attributes(const char *server_ipv4_addr, uint16_t server_port, Nfs__FHandle fhandle, Nfs__AttrStat *result);

int nfs_procedure_2_set_file_attributes(const char *server_ipv4_addr, uint16_t server_port, Nfs__SAttrArgs sattrargs, Nfs__AttrStat *result);

int nfs_procedure_4_look_up_file_name(const char *server_ipv4_addr, uint16_t server_port, Nfs__DirOpArgs diropargs, Nfs__DirOpRes *result);

int nfs_procedure_6_read_from_file(const char *server_ipv4_addr, uint16_t server_port, Nfs__ReadArgs readargs, Nfs__ReadRes *result);

int nfs_procedure_8_write_to_file(const char *server_ipv4_addr, uint16_t server_port, Nfs__WriteArgs writeargs, Nfs__AttrStat *result);

int nfs_procedure_9_create_file(const char *server_ipv4_addr, uint16_t server_port, Nfs__CreateArgs createargs, Nfs__DirOpRes *result);

int nfs_procedure_10_remove_file(const char *server_ipv4_addr, uint16_t server_port, Nfs__DirOpArgs diropargs, Nfs__NfsStat *result);

int nfs_procedure_11_rename_file(const char *server_ipv4_addr, uint16_t server_port, Nfs__RenameArgs renameargs, Nfs__NfsStat *result);

int nfs_procedure_13_create_symbolic_link(const char *server_ipv4_addr, uint16_t server_port, Nfs__SymLinkArgs symlinkargs, Nfs__NfsStat *result);

int nfs_procedure_14_create_directory(const char *server_ipv4_addr, uint16_t server_port, Nfs__CreateArgs createargs, Nfs__DirOpRes *result);

int nfs_procedure_15_remove_directory(const char *server_ipv4_addr, uint16_t server_port, Nfs__DirOpArgs diropargs, Nfs__NfsStat *result);

int nfs_procedure_16_read_from_directory(const char *server_ipv4_addr, uint16_t server_port, Nfs__ReadDirArgs readdirargs, Nfs__ReadDirRes *result);

int nfs_procedure_17_get_filesystem_attributes(const char *server_ipv4_addr, uint16_t server_port, Nfs__FHandle fhandle, Nfs__StatFsRes *result);

#endif /* nfs_client__header__INCLUDED */