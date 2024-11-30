#ifndef common_validation__header__INCLUDED
#define common_validation__header__INCLUDED

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/serialization/nfs/nfs.pb-c.h"

void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype);

void check_equal_fattr(Nfs__FAttr *fattr1, Nfs__FAttr *fattr2);

#endif /* common_validation__header__INCLUDED */