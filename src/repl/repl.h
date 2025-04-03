#ifndef repl__header__INCLUDED
#define repl__header__INCLUDED

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "src/serialization/nfs/nfs.pb-c.h"

#include "filesystem_dag/filesystem_dag.h"

#include "src/common_rpc/rpc_connection_context.h"

/*
 * Nfs Client state.
 */

extern RpcConnectionContext *rpc_connection_context;

extern DAGNode *filesystem_dag_root;
extern DAGNode *cwd_node;

extern TransportProtocol chosen_transport_protocol;

int is_filesystem_mounted(void);

#endif /* repl__header__INCLUDED */