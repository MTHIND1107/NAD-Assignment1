#include "fileHandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int loadFile(const char* filename, char** buffer, size_t* size)
{
    FILE* file = fopen(filename, "rb");
    if (!file) 
    {
        perror("Error opening file");
        return -1;
    }

    fseek(file, 0, SEEK_END); 
    *size = ftell(file);  
    fseek(file, 0, SEEK_SET); 

    *buffer = (char*)malloc(*size);
    if (!*buffer) 
    {  
        fclose(file); 
        return -1;
    }

}

int saveFile(const char* filename, const char* buffer, size_t size) 
{

}

int SendFile(void) {

}

int ReceiveFile(void) {

}

int VerifyFile(void) {

}
