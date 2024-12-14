#include "path_building.h"

/*
* Given the absolute path of the containing directory, and a file name of the file inside it,
* returns the absolute path of that file.
* The absolute path of the desired file is 'directory_absolute_path/file_name'.
*
* The user of this function takes the responsibility to free the returned string.
*/
char *get_file_absolute_path(char *directory_absolute_path, char *file_name) {
    int file_absolute_path_length = strlen(directory_absolute_path) + 1 + strlen(file_name);
    char *concatenation_buffer = malloc(file_absolute_path_length + 1);   // create a string with enough space, +1 for termination character

    strcpy(concatenation_buffer, directory_absolute_path);    // move the directory absolute path to concatenation_buffer
    concatenation_buffer = strcat(concatenation_buffer, "/"); // add a slash at end

    return strcat(concatenation_buffer, file_name);
}