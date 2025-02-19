/*
 * FILE: fileHandler.cpp
 * PROJECT: Reliable UDP File Transfer
 * PROGRAMMER: Manreet & Bhawanjeet
 * FIRST VERSION: 15/02/2025
 * DESCRIPTION:
 * This source file implements file handling functions for the Reliable UDP
 * file transfer system. It includes functions to read and write files, generate
 * metadata packets, and perform integrity verification using CRC32.
 */
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
const int size = 256;



static uint32_t crc32_table[size];
//
//Source: https://gist.github.com/timepp/1f678e200d9e0f2a043a9ec6b3690635

void init_crc32_table(void) {
    for (uint32_t i = 0; i < size; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (crc & 1 ? POLYNOMIAL : 0);
        }
        crc32_table[i] = crc;
    }
}

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
    double duration = endTime - startTime; // Time difference in seconds

    // Debugging prints to check values
    printf("Start Time: %.6f, End Time: %.6f, Duration: %.6f\n", startTime, endTime, duration);

    if (duration <= 0.0) {
        printf("Error: Invalid duration (%.6f seconds)\n", duration);
        return 0.0;  // Prevent division by zero or negative speeds
    }

    // Convert bytes to bits and calculate speed in Mbps
    double speed = (fileSize * 8.0) / (duration * 1e6); // Convert to Mbps

    // Additional debugging to show intermediate results
    printf("File Size: %zu bytes, Speed in bits/sec: %.2f\n", fileSize, (fileSize * 8.0) / duration);
    printf("Transfer Speed: %.2f Mbps\n", speed);

    return speed;
}
//Creating a metadata packet
void createMetadataPacket(const char* filename, size_t fileSize, uint32_t crc, bool isLast, char* packet, size_t* packetSize, size_t offset) {
    FileMetadata metadata;
    strncpy(metadata.filename, filename, sizeof(metadata.filename) - 1);
    metadata.fileSize = fileSize;
    metadata.crc = crc;
    metadata.isLastPacket = isLast;

    size_t totalMetadataSize = sizeof(FileMetadata);
    size_t remainingSize = totalMetadataSize - offset;
    size_t chunkSize = (remainingSize < size) ? remainingSize : size;
    memcpy(packet, ((char*)&metadata) + offset, chunkSize);
    *packetSize = chunkSize;
}
//Get the metadata packet
bool extractMetadataPacket(const char* packet, size_t bytesRead, FileMetadata* metadata, char* metadataBuffer, size_t* receivedMetaOffset) {
    if (!packet || !metadata || !metadataBuffer || !receivedMetaOffset) {
        return false;
    }
    memcpy(metadataBuffer + *receivedMetaOffset, packet, bytesRead);
    *receivedMetaOffset += bytesRead;

    // Check if we have received the full metadata structure
    if (*receivedMetaOffset >= sizeof(FileMetadata)) {
        memcpy(metadata, metadataBuffer, sizeof(FileMetadata));
        *receivedMetaOffset = 0;
        return true;
    }
    return false;
}
// Function to create a data packet
size_t createDataPacket(const char* fileBuffer, size_t fileSize, size_t currentOffset, char* tempBuffer, size_t maxPacketSize, bool isLastPacket) {
    size_t remainingSize = fileSize - currentOffset;
    size_t packetSize = (remainingSize < maxPacketSize) ? remainingSize : maxPacketSize;

    memcpy(tempBuffer, fileBuffer + currentOffset, packetSize);
    return packetSize;  // Return the actual packet size
}


bool VerifyFile(const char* filename, uint32_t expectedCRC) {
        char* buffer;
        size_t fileSize;
        if (loadFile(filename, &buffer, &fileSize) < 0) return false;

        uint32_t computedCRC = computeCRC32(buffer, fileSize);
        free(buffer);

        return computedCRC == expectedCRC;
    
}
