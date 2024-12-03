#ifndef procedure_validation__header__INCLUDED
#define procedure_validation__header__INCLUDED

#include <stdlib.h>
#include <sys/types.h>

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

#include "common_validation.h"

// MOUNTPROC_MNT validation
Mount__FhStatus *mount_directory_success(char *directory_absolute_path);

void mount_directory_fail(char *directory_absolute_path, Mount__Stat non_mnt_ok_status);

// NFSPROC_GETATTR validation
Nfs__AttrStat *get_attributes_success(Nfs__FHandle file_fhandle, Nfs__FType expected_ftype);

void get_attributes_fail(Nfs__FHandle file_fhandle, Nfs__Stat non_nfs_ok_status);

// NFSPROC_SETATTR validation
Nfs__AttrStat *set_attributes_success(Nfs__FHandle *file_fhandle, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype);

void set_attributes_fail(Nfs__FHandle *file_fhandle, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status);

// NFSPROC_LOOKUP validation
Nfs__DirOpRes *lookup_file_or_directory_success(Nfs__FHandle *directory_fhandle, char *filename, Nfs__FType expected_ftype);

void lookup_file_or_directory_fail(Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status);

// NFSPROC_READ validation
Nfs__ReadRes *read_from_file_success(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__FAttr *attributes_before_read, uint32_t expected_read_size, uint8_t *expected_read_content);

void read_from_file_fail(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__Stat non_nfs_ok_status);

// NFSPROC_WRITE validation
Nfs__AttrStat *write_to_file_success(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__FType ftype);

void write_to_file_fail(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__Stat non_nfs_ok_status);

// NFSPROC_CREATE validation
Nfs__DirOpRes *create_file_success(Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype);

void create_file_fail(Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status);

// NFSPROC_READDIR validation
Nfs__ReadDirRes *read_from_directory_success(Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count, int expected_number_of_entries, char *expected_filenames[]);

void read_from_directory_fail(Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count, Nfs__Stat non_nfs_ok_status);

#endif /* procedure_validation__header__INCLUDED */