#ifndef common_validation__header__INCLUDED
#define common_validation__header__INCLUDED

#include <string.h>

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/serialization/nfs/nfs.pb-c.h"

void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype);

void check_equal_fattr(Nfs__FAttr *fattr1, Nfs__FAttr *fattr2);

void check_fattr_update(Nfs__FAttr *updated_fattr, Nfs__SAttr *sattr);

#endif /* common_validation__header__INCLUDED */