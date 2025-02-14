#include "fileHandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <chrono>
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
uint32_t computeCRC32(const char* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint8_t)data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 * (crc & 1));
        }
    }
    return ~crc;
}

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
    fseek(file, 0, SEEK_SET); // Move the file pointer back to the beginning


    *buffer = (char*)malloc(*size);
    if (!*buffer) 
    {  
        fclose(file);
        return -1;
    }

    fread(*buffer, 1, *size, file);  // Read file
    fclose(file); 
    return 0;

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

    size_t written = fwrite(buffer, 1, size, file);
    fclose(file);  // Close the file after writing
    return (written == size) ? 0 : -1;
}

// Calculate transfer speed in Mbps
double calculateTransferSpeed(double startTime, double endTime, size_t fileSize) 
{
    double duration = endTime - startTime;
    if (duration <= 0) return 0.0; // Prevent division by zero

    return (fileSize * 8.0) / (duration * 1e6); // Convert to Mbps
}
struct FileMetadata {
    char filename[256];
    size_t fileSize;
    uint32_t crc;
    bool isLastPacket;
};

//Creating a metadata packet
void createMetadataPacket(const char* filename, size_t fileSize, uint32_t crc, char* packet, size_t* packetSize) {
    FileMetadata metadata;
    strncpy(metadata.filename, filename, sizeof(metadata.filename) - 1);
    metadata.fileSize = fileSize;
    metadata.crc = crc;
    metadata.isLastPacket = false;

    memcpy(packet, &metadata, sizeof(FileMetadata));
    *packetSize = sizeof(FileMetadata);
}
//Get the metadata packet
bool extractMetadataPacket(const char* packet, FileMetadata* metadata) {
    if (!packet || !metadata) {
        return false;
    }
    memcpy(metadata, packet, sizeof(FileMetadata));
    return true;
}



int VerifyFile(void) {
    return 0;
}
