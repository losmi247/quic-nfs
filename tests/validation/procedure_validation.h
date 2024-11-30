#ifndef procedure_validation__header__INCLUDED
#define procedure_validation__header__INCLUDED

#include <stdlib.h>

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

#include "common_validation.h"

Mount__FhStatus *mount_directory(char *directory_absolute_path);

Nfs__DirOpRes *lookup_file_or_directory(Nfs__FHandle *directory_fhandle, char *filename, Nfs__FType expected_ftype);

Nfs__ReadRes *read_from_file(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__FAttr *attributes_before_read);

Nfs__AttrStat *write_to_file(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer);

#endif /* procedure_validation__header__INCLUDED */