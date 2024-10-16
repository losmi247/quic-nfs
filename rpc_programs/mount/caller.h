#ifndef mount__caller__header__INCLUDED
#define mount__caller__header__INCLUDED

#include <protobuf-c/protobuf-c.h>
#include "serialization/mount/mount.pb-c.h"
#include "serialization/google_protos/any.pb-c.h";

Google__Protobuf__Any *call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

#endif /* mount__caller__header__INCLUDED */