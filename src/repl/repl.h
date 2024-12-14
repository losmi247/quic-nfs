#ifndef repl__header__INCLUDED
#define repl__header__INCLUDED

#include <stdint.h>
#include <src/serialization/nfs/nfs.pb-c.h>

#include "filesystem_dag/filesystem_dag.h"

/*
* Nfs Client state.
*/

extern char *server_ipv4_addr;
extern uint16_t server_port_number;

extern DAGNode *filesystem_dag_root;
extern DAGNode *cwd_node;

#endif /* repl__header__INCLUDED */