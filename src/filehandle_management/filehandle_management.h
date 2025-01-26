#ifndef filehandle_management__header__INCLUDED
#define filehandle_management__header__INCLUDED

#include "src/serialization/nfs_fh/nfs_fh.pb-c.h"

NfsFh__NfsFileHandle deep_copy_nfs_filehandle(NfsFh__NfsFileHandle *nfs_filehandle);

#endif /* filehandle_management__header__INCLUDED */