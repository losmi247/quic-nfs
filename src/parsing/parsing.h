#ifndef parsing__header__INCLUDED
#define parsing__header__INCLUDED

#include "stdint.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "src/serialization/nfs/nfs.pb-c.h"
#include "src/serialization/mount/mount.pb-c.h"

uint16_t parse_port_number(char *port_number_string);

char *mount_stat_to_string(Mount__Stat stat);

char *nfs_stat_to_string(Nfs__Stat stat);

#endif /* parsing__header__INCLUDED */