#ifndef repl__header__INCLUDED
#define repl__header__INCLUDED

#include <stdint.h>
#include <src/serialization/nfs/nfs.pb-c.h>

/*
* Nfs Client state.
*/

extern char *server_ipv4_addr;
extern uint16_t server_port_number;

extern char *cwd_absolute_path;
extern Nfs__FHandle *cwd_filehandle;

#endif /* repl__header__INCLUDED */