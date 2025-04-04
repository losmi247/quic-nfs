#ifndef procedure_validation__header__INCLUDED
#define procedure_validation__header__INCLUDED

#include <stdlib.h>
#include <sys/types.h>

#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

#include "common_validation.h"

#include "tests/test_common.h"

// MOUNTPROC_MNT validation
Mount__FhStatus *mount_directory_success(RpcConnectionContext *rpc_connection_context, char *directory_absolute_path);

void mount_directory_fail(RpcConnectionContext *rpc_connection_context, char *directory_absolute_path,
                          Mount__Stat non_mnt_ok_status);

// NFSPROC_GETATTR validation
Nfs__AttrStat *get_attributes_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle file_fhandle,
                                      Nfs__FType expected_ftype);

void get_attributes_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle file_fhandle,
                         Nfs__Stat non_nfs_ok_status);

// NFSPROC_SETATTR validation
Nfs__AttrStat *set_attributes_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle,
                                      mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime,
                                      Nfs__TimeVal *mtime, Nfs__FType ftype);

void set_attributes_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, mode_t mode,
                         uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime,
                         Nfs__Stat non_nfs_ok_status);

// NFSPROC_LOOKUP validation
Nfs__DirOpRes *lookup_file_or_directory_success(RpcConnectionContext *rpc_connection_context,
                                                Nfs__FHandle *directory_fhandle, char *filename,
                                                Nfs__FType expected_ftype);

void lookup_file_or_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                                   char *filename, Nfs__Stat non_nfs_ok_status);

// NFSPROC_READLINK validation
Nfs__ReadLinkRes *read_from_symbolic_link_success(RpcConnectionContext *rpc_connection_context,
                                                  Nfs__FHandle *file_fhandle, char *expected_target_path);

void read_from_symbolic_link_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle,
                                  Nfs__Stat non_nfs_ok_status);

// NFSPROC_READ validation
Nfs__ReadRes *read_from_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle,
                                     uint32_t offset, uint32_t byte_count, Nfs__FAttr *attributes_before_read,
                                     uint32_t expected_read_size, uint8_t *expected_read_content);

void read_from_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset,
                         uint32_t byte_count, Nfs__Stat non_nfs_ok_status);

// NFSPROC_WRITE validation
Nfs__AttrStat *write_to_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle,
                                     uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__FType ftype);

void write_to_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset,
                        uint32_t byte_count, uint8_t *source_buffer, Nfs__Stat non_nfs_ok_status);

// NFSPROC_CREATE validation
Nfs__DirOpRes *create_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                                   char *filename, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime,
                                   Nfs__TimeVal *mtime, Nfs__FType ftype);

void create_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename,
                      mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime,
                      Nfs__Stat non_nfs_ok_status);

// NFSPROC_REMOVE validation
Nfs__NfsStat *remove_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                                  char *filename);

void remove_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename,
                      Nfs__Stat non_nfs_ok_status);

// NFSPROC_RENAME validation
Nfs__NfsStat *rename_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *from_directory_fhandle,
                                  char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename);

void rename_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *from_directory_fhandle,
                      char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename,
                      Nfs__Stat non_nfs_ok_status);

// NFSPROC_LINK validation
Nfs__NfsStat *create_link_to_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *target_file,
                                          Nfs__FHandle *directory_fhandle, char *filename);

void create_link_to_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *target_file,
                              Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status);

// NFSPROC_SYMLINK validation
Nfs__NfsStat *create_symbolic_link_success(RpcConnectionContext *rpc_connection_context,
                                           Nfs__FHandle *directory_fhandle, char *filename, Nfs__Path *target);

void create_symbolic_link_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                               char *filename, Nfs__Path *target, Nfs__Stat non_nfs_ok_status);

// NFSPROC_MKDIR validation
Nfs__DirOpRes *create_directory_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                                        char *filename, mode_t mode, uid_t uid, uid_t gid, Nfs__TimeVal *atime,
                                        Nfs__TimeVal *mtime, Nfs__FType ftype);

void create_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                           char *filename, mode_t mode, uid_t uid, uid_t gid, Nfs__TimeVal *atime, Nfs__TimeVal *mtime,
                           Nfs__Stat non_nfs_ok_status);

// NFSPROC_RMDIR validation
Nfs__NfsStat *remove_directory_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                                       char *filename);

void remove_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                           char *filename, Nfs__Stat non_nfs_ok_status);

// NFSPROC_READDIR validation
Nfs__ReadDirRes *read_from_directory_success(RpcConnectionContext *rpc_connection_context,
                                             Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count,
                                             int expected_number_of_entries, char *expected_filenames[]);

void read_from_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle,
                              uint64_t cookie, uint32_t byte_count, Nfs__Stat non_nfs_ok_status);

// NFSPROC_STATFS validation
Nfs__StatFsRes *get_filesystem_attributes_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle);

void get_filesystem_attributes_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle,
                                    Nfs__Stat non_nfs_ok_status);

#endif /* procedure_validation__header__INCLUDED */