#include "fileHandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#pragma warning(disable: 4996)


#define PACKET_SIZE 1024
#define CHECKSUM_SIZE 4 // CRC32 produces 4-byte checksum
#define POLYNOMIAL 0xEDB88320  // Standard CRC-32 polynomial

static uint32_t crc32_table[256];
//MAKE SURE TO CITE THIS
/*
* Name: init_crc32_table
* Parameteres: None
* Returns: Nothing
* Description: Initialized the CRC table
*/
void init_crc32_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (crc & 1 ? POLYNOMIAL : 0);
        }
        crc32_table[i] = crc;
    }
}
/*
* Name: compute_crc32
* Parameteres: const uint8_t* data, size_t length
* Returns: uint32_t
* Description: compute the CRC32 using table
*/
uint32_t compute_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = ~0;  // Start with all bits set (0xFFFFFFFF)
    for (size_t i = 0; i < length; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;  // Final XOR with 0xFFFFFFFF
}

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

/*
* Name: saveFile
* Parameteres: const char* filename, const char* buffer, size_t size
* Returns: int
* Description: Saves the file to the disk
*/
int saveFile(const char* filename, const char* buffer, size_t size) 
{
    FILE* file = fopen(filename, "wb");   // Open file in binary write mode
    if (!file) 
    {
        perror("Error opening file"); // Print error if file creation fails
        return -1;
    }

    fwrite(buffer, 1, size, file);   // Write buffer contents to the file
    fclose(file);  // Close the file after writing
    return 0;    // Return success
}
/*
* Function Name: SendFile
* Parameters: filename(name of the file to be sent), destIP(IP where the file is to be sent),
*             destPort(Port where the file is sent for communication),
* 
*/
int SendFile(const char* filename, const char* destIP, int destPort) { 
    char* buffer;
    size_t fileSize;
    if (loadFile(filename, &buffer, &fileSize) < 0) { //call the function loadFile to get the contents in buffer and file size.
        return -1;
    }
    
}

int ReceiveFile(void) {
    return 0;
}

int VerifyFile(void) {
    return 0;
}
