#include "fileHandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int loadFile(const char* filename, char** buffer, size_t* size)
{
    FILE* file = fopen(filename, "rb");// Open the file in binary read mode
    if (!file) 
    {
        perror("Error opening file");// Print error message if the file cannot be opened
        return -1;
    }

    fseek(file, 0, SEEK_END);  // Move the file pointer to the end to determine file size
    *size = ftell(file);   // Get the file size in bytes
    fseek(file, 0, SEEK_SET); // Move the file pointer back to the beginning


    *buffer = (char*)malloc(*size);
    if (!*buffer) // Check if memory allocation was successful
    {  
        fclose(file); // Close the file before returning an error
        return -1;
    }

    fread(*buffer, 1, *size, file);  // Read file contents into the allocated buffer
    fclose(file);  // Read file contents into the allocated buffer
    return 0;   // Return success

}

int saveFile(const char* filename, const char* buffer, size_t size) 
{
    FILE* file = fopen(filename, "wb");  
    if (!file) {
        perror("Error opening file"); 
        return -1;
    }

}

int SendFile(void) {

}

int ReceiveFile(void) {

}

int VerifyFile(void) {

}
