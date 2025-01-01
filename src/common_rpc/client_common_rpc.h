#ifndef client_common_rpc__header__INCLUDED
#define client_common_rpc__header__INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/parsing/parsing.h"

/*
* Functions implemented in client_common_rpc.c file.
*/

int validate_successful_accepted_reply(Rpc__RpcMsg *rpc_reply);

#endif /* client_common_rpc__header__INCLUDED */