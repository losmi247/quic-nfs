#ifndef handlers__header__INCLUDED
#define handlers__header__INCLUDED

#include "src/repl/repl.h"

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"
#include "src/common_permissions/common_permissions.h"

#include "src/parsing/parsing.h"
#include "src/repl/filehandle_management.h"
#include "src/common_rpc/rpc_connection_context.h"

#include "src/transport/transport_common.h"

int handle_mount(char *server_ip, uint16_t server_port, char *remote_absolute_path);

int handle_ls();

int handle_cd(char *directory_name);

int handle_touch(char *file_name);

int handle_mkdir(char *directory_name);

int handle_cat(char *file_name);

int handle_echo(char *text, char *file_name);

int handle_rm(char *file_name);

#endif /* handlers__header__INCLUDED */