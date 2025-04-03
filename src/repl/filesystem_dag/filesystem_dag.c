#include "filesystem_dag.h"

/*
 * Creates a new node in the filesystem DAG.
 *
 * The user of this function takes the responsibility to free the allocated 'absolute_path', fhandle->nfs_filehandle,
 * fhandle, and the DAGNode.
 */
DAGNode *create_dag_node(const char *absolute_path, Nfs__FType ftype, Nfs__FHandle *fhandle, int is_root) {
    DAGNode *node = malloc(sizeof(DAGNode));
    if (!node) {
        return NULL;
    }

    node->absolute_path = strdup(absolute_path);
    node->ftype = ftype;
    node->fhandle = fhandle;

    node->parent = NULL;
    node->is_root = is_root;

    node->children = NULL;
    node->child_count = 0;

    node->ref_count = 1;

    return node;
}

/*
 * Adds the 'child' DAGNode to children of the 'parent' DAGNode.
 *
 * Returns 0 on success and > 0 on failure.
 */
int add_child(DAGNode *parent, DAGNode *child) {
    if (!parent || parent->ftype != NFS__FTYPE__NFDIR || !child) {
        return 1;
    }

    // allocate space for another child node pointer
    parent->children = realloc(parent->children, sizeof(DAGNode *) * (parent->child_count + 1));
    if (!parent->children) {
        return 1;
    }

    parent->children[parent->child_count] = child;
    parent->child_count++;

    child->parent = parent;
    child->ref_count++;

    return 0;
}

/*
 * Decrements the reference count of the given DAG node, and if it reaches zero,
 * frees heap allocated space in the given DAGNode and the DAGNode itself.
 *
 * Note that this function does not recursively free children nodes, it only frees
 * the array of pointers (to children) that this node holds.
 *
 * Does nothing if the given DAGNode is NULL.
 */
void decrement_ref_count(DAGNode *node) {
    if (node == NULL) {
        return;
    }

    if (--node->ref_count == 0) {
        free(node->absolute_path);
        free(node->fhandle->nfs_filehandle);
        free(node->fhandle);
        free(node->children);

        free(node);
    }
}

/*
 * Deallocates all DAG nodes and heap allocated fields in them, in the given DAG.
 *
 * Does nothing if the given DAGNode is NULL.
 */
void clean_up_dag(DAGNode *node) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < node->child_count; i++) {
        clean_up_dag(node->children[i]);
    }

    decrement_ref_count(node);
}
