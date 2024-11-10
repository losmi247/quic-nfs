#include "file_management.h"

/*
* Given the absolute path of a directory or a file, places its inode number in 
* 'inode_number' argument. 
*
* Returns 0 on success, and > 0 on failure.
*/
int get_inode_number(char *file_or_directory_absolute_path, ino_t *inode_number) {
    int fd = open(file_or_directory_absolute_path, O_RDONLY);
    if(fd < 0) {
        perror("Failed opening the file");
        return 1;
    }

    struct stat file_stat;
    if(fstat(fd, &file_stat) < 0) {
        perror("Failed retrieving file stats");
        return 2;
    }

    *inode_number = file_stat.st_ino;

    return 0;
}