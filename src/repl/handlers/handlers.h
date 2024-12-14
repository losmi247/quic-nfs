#ifndef handlers__header__INCLUDED
#define handlers__header__INCLUDED

#include "src/repl/repl.h"

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

#include "src/parsing/parsing.h"
#include "src/repl/filehandle_management.h"

void handle_mount(const char *server_ip, uint16_t server_port, char *remote_absolute_path);

void handle_ls();

void handle_cd(char *directory_name);

#endif /* handlers__header__INCLUDED */