#ifndef soft_links__HEADER__INCLUDED
#define soft_links__HEADER__INCLUDED

#include "src/serialization/nfs/nfs.pb-c.h"

#include "src/repl/filesystem_dag/filesystem_dag.h"

#include "src/filehandle_management/filehandle_management.h"

#include "src/common_rpc/rpc_connection_context.h"

Nfs__FHandle *follow_symbolic_links(char *command_name, RpcConnectionContext *rpc_connection_context,
                                    DAGNode *filesystem_dag_root, DAGNode *cwd_node, Nfs__FHandle *start_file_fhandle,
                                    Nfs__FType start_file_type);

#endif /* soft_links__HEADER__INCLUDED */