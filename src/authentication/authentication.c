#include "authentication.h"

/*
 * Creates a OpaqueAuth with AUTH_NONE flavor and returns it.
 *
 * The user of this function takes the responsibility to deallocate the
 * constructed OpaqueAuth using free_opaque_auth function.
 */
Rpc__OpaqueAuth *create_auth_none_opaque_auth(void) {
    Rpc__OpaqueAuth *opaque_auth = malloc(sizeof(Rpc__OpaqueAuth));
    rpc__opaque_auth__init(opaque_auth);

    opaque_auth->flavor = RPC__AUTH_FLAVOR__AUTH_NONE;
    opaque_auth->body_case = RPC__OPAQUE_AUTH__BODY_EMPTY;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    opaque_auth->empty = empty;

    return opaque_auth;
}

/*
 * Creates a OpaqueAuth with AUTH_SYS flavor and the given machine name, uid,
 * gid, and collection of gids.
 *
 * The user of this function takes the responsibility to deallocate the
 * constructed OpaqueAuth using free_opaque_auth function.
 */
Rpc__OpaqueAuth *create_auth_sys_opaque_auth(char *machine_name, uint32_t uid, uint32_t gid, uint32_t number_of_gids,
                                             uint32_t *gids) {
    Rpc__OpaqueAuth *opaque_auth = malloc(sizeof(Rpc__OpaqueAuth));
    rpc__opaque_auth__init(opaque_auth);

    opaque_auth->flavor = RPC__AUTH_FLAVOR__AUTH_SYS;
    opaque_auth->body_case = RPC__OPAQUE_AUTH__BODY_AUTH_SYS;

    Rpc__AuthSysParams *authsysparams = malloc(sizeof(Rpc__AuthSysParams));
    rpc__auth_sys_params__init(authsysparams);
    authsysparams->timestamp = time(NULL);

    if (strlen(machine_name) <= MAX_MACHINENAME_LEN) {
        authsysparams->machinename = strdup(machine_name);
    } else {
        // truncate the name down to MAX_MACHINENAME_LEN characters
        char name[MAX_MACHINENAME_LEN + 1];
        memcpy(name, machine_name, MAX_MACHINENAME_LEN);
        name[MAX_MACHINENAME_LEN] = '\0';

        authsysparams->machinename = strdup(name);
    }

    authsysparams->uid = uid;
    authsysparams->gid = gid;

    authsysparams->n_gids = number_of_gids <= MAX_N_GIDS ? number_of_gids : MAX_N_GIDS;
    authsysparams->gids = malloc(sizeof(uint32_t) * authsysparams->n_gids);
    for (int offset = 0; offset < authsysparams->n_gids; offset++) {
        authsysparams->gids[offset] = gids[offset];
    }

    opaque_auth->auth_sys = authsysparams;

    return opaque_auth;
}

/*
 * Deallocates all heap-allocated fields of the given OpaqueAuth and
 * the OpaqueAuth itself.
 * Works only for OpaqueAuth's with AUTH_NONE or AUTH_SYS flavor.
 *
 * Does nothing if the opaque_auth is NULL.
 */
void free_opaque_auth(Rpc__OpaqueAuth *opaque_auth) {
    if (opaque_auth == NULL) {
        return;
    }

    if (opaque_auth->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        free(opaque_auth->empty);
        free(opaque_auth);
    } else if (opaque_auth->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        if (opaque_auth->auth_sys == NULL) {
            free(opaque_auth);
            return;
        }

        Rpc__AuthSysParams *authsysparams = opaque_auth->auth_sys;
        free(authsysparams->machinename);

        free(authsysparams->gids);

        free(authsysparams);

        free(opaque_auth);
    }
}
