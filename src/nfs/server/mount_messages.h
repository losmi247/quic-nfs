#ifndef mount_messages__header__INCLUDED
#define mount_messages__header__INCLUDED

#include <stdlib.h>

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"
#include "src/serialization/mount/mount.pb-c.h"

Mount__FhStatus create_default_case_fh_status(int non_zero_status);

#endif /* mount_messages__header__INCLUDED */