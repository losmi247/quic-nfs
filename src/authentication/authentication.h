#ifndef authentication__header__INCLUDED
#define authentication__header__INCLUDED

#define MAX_MACHINENAME_LEN 255 // max length of the machinename string in AuthSysParams
#define MAX_N_GIDS 32           // max number of gids in AuthSysParams

#include "src/serialization/rpc/rpc.pb-c.h"

#include "time.h"
#include <stdlib.h>
#include <string.h>

Rpc__OpaqueAuth *create_auth_none_opaque_auth(void);

Rpc__OpaqueAuth *create_auth_sys_opaque_auth(char *machine_name, uint32_t uid, uint32_t gid, uint32_t number_of_gids,
                                             uint32_t *gids);

void free_opaque_auth(Rpc__OpaqueAuth *opaque_auth);

#endif /* authentication__header__INCLUDED */