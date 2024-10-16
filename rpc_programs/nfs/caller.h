#ifndef nfs__caller__header__INCLUDED
#define nfs__caller__header__INCLUDED

#include <protobuf-c/protobuf-c.h>
#include "serialization/nfs/nfs.pb-c.h"; // doesn't exist yet

Google__Protobuf__Any *call_nfs(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

#endif /* nfs__caller__header__INCLUDED */