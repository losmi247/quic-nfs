#include "nfsproc.h"

/*
* Runs the NFSPROC_READDIR procedure (16).
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_16_read_from_directory(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/ReadDirArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: Expected nfs/ReadDirArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__ReadDirArgs *readdirargs = nfs__read_dir_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(readdirargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: Failed to unpack ReadDirArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(readdirargs->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: 'dir' in ReadDirArgs is null\n");

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(readdirargs->cookie == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: 'cookie' in ReadDirArgs is null\n");

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = readdirargs->dir;
    if(directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: FHandle->nfs_filehandle is null\n");

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *directory_nfs_filehandle = directory_fhandle->nfs_filehandle;
    ino_t inode_number = directory_nfs_filehandle->inode_number;

    char *directory_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: failed to decode inode number %ld back to a directory\n", inode_number);

        // build the procedure results
        Nfs__ReadDirRes *readdirres = create_default_case_read_dir_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t readdirres_size = nfs__read_dir_res__get_packed_size(readdirres);
        uint8_t *readdirres_buffer = malloc(readdirres_size);
        nfs__read_dir_res__pack(readdirres, readdirres_buffer);

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);
        free(readdirres->nfs_status);
        free(readdirres->default_case);
        free(readdirres);

        return wrap_procedure_results_in_successful_accepted_reply(readdirres_size, readdirres_buffer, "nfs/ReadDirRes");
    }

    // get the attributes of this directory before the read, to check that it is actually a directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(directory_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: failed getting file attributes for file at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    // only directories can be read using READDIR
    if(fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return ReadDirRes with 'non-directory specified in a directory operation' status
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: a non-directory '%s' was specified for 'readdir' which is a directory operation\n", directory_absolute_path);

        // build the procedure results
        Nfs__ReadDirRes *readdirres = create_default_case_read_dir_res(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t readdirres_size = nfs__read_dir_res__get_packed_size(readdirres);
        uint8_t *readdirres_buffer = malloc(readdirres_size);
        nfs__read_dir_res__pack(readdirres, readdirres_buffer);

        clean_up_fattr(&fattr);
        nfs__read_dir_args__free_unpacked(readdirargs, NULL);
        free(readdirres->nfs_status);
        free(readdirres->default_case);
        free(readdirres);

        return wrap_procedure_results_in_successful_accepted_reply(readdirres_size, readdirres_buffer, "nfs/ReadDirRes");
    }
    clean_up_fattr(&fattr);

    // read entries from the directory
    Nfs__DirectoryEntriesList *directory_entries = NULL;
    int end_of_stream = 0;
    error_code = read_from_directory(directory_absolute_path, readdirargs->cookie->value, readdirargs->count, &directory_entries, &end_of_stream);
    if(error_code > 0) {
        // we failed reading directory entries
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: failed reading directory entries for directory at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    
    // build the procedure results
    Nfs__ReadDirRes readdirres = NFS__READ_DIR_RES__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;

    readdirres.nfs_status = &nfs_status;
    readdirres.body_case = NFS__READ_DIR_RES__BODY_READDIROK;

    Nfs__ReadDirOk readdirok = NFS__READ_DIR_OK__INIT;
    readdirok.entries = directory_entries;
    readdirok.eof = end_of_stream;

    readdirres.readdirok = &readdirok;

    // serialize the procedure results
    size_t readdirres_size = nfs__read_dir_res__get_packed_size(&readdirres);
    uint8_t *readdirres_buffer = malloc(readdirres_size);
    nfs__read_dir_res__pack(&readdirres, readdirres_buffer);

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(readdirres_size, readdirres_buffer, "nfs/ReadDirRes");

    nfs__read_dir_args__free_unpacked(readdirargs, NULL);

    clean_up_directory_entries_list(directory_entries);

    return accepted_reply;
}