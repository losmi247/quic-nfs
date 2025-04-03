#ifndef filesystem_dag__header__INCLUDED
#define filesystem_dag__header__INCLUDED

#include <stdlib.h>
#include <string.h>

#include "src/serialization/nfs/nfs.pb-c.h"

typedef struct DAGNode {
    char *absolute_path; // absolute path of this file/directory on the server machine
    Nfs__FType ftype;
    Nfs__FHandle *fhandle; // the filehandle obtained from the server for this file/directory

    struct DAGNode *parent; // pointer to the parent node (NULL for root)
    int is_root;            // flag indicating if the node is the root

    struct DAGNode **children; // array of child nodes (NULL for files)
    size_t child_count;        // number of children

    size_t ref_count; // reference count used when freeing the entire DAG
} DAGNode;

DAGNode *create_dag_node(const char *absolute_path, Nfs__FType ftype, Nfs__FHandle *fhandle, int is_root);

int add_child(DAGNode *parent, DAGNode *child);

void clean_up_dag(DAGNode *node);

#endif /* filesystem_dag__header__INCLUDED */