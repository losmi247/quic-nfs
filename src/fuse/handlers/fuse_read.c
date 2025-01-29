#include "handlers.h"

int nfs_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf( "--> Trying to read %s, %lu, %lu\n", path, offset, size );
	
	char file54Text[] = "Hello World From File54!";
	char file349Text[] = "Hello World From File349!";
	char *selectedText = NULL;
	
	// ... //
	
	if ( strcmp( path, "/file54" ) == 0 )
		selectedText = file54Text;
	else if ( strcmp( path, "/file349" ) == 0 )
		selectedText = file349Text;
	else
		return -1;
	
	// ... //
	
	memcpy( buffer, selectedText + offset, size );
		
	return strlen( selectedText ) - offset;
}