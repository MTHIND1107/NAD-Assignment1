/*
 * FILE: fileHandler.h
 * PROJECT: Reliable UDP File Transfer
 * PROGRAMMER: Bhawanjeet & Manreet
 * FIRST VERSION: 15/02/2025
 * DESCRIPTION:
 * This header file declares functions for file handling operations, including
 * loading, saving, metadata management, and integrity verification.
 */
#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PACKET_SIZE 1024
#define CHECKSUM_SIZE 4  // CRC32 checksum size

typedef struct {
    char filename[256];  // Adjust size as needed
    size_t fileSize;
    uint32_t crc;
    bool isLast;
} FileMetadata;

void init_crc32_table(void);
uint32_t computeCRC32(const char* data, size_t size);
int loadFile(const char* filename, char** buffer, size_t* size);
int saveFile(const char* filename, const char* buffer, size_t size);
double calculateTransferSpeed(double startTime, double endTime, size_t fileSize);
void createMetadataPacket(const char* filename, size_t fileSize, uint32_t crc, bool isLast, char* packet, size_t* packetSize);
bool extractMetadataPacket(const char* packet, FileMetadata* metadata);
size_t createDataPacket(const char* fileBuffer, size_t fileSize, size_t currentOffset, char* tempBuffer, size_t maxPacketSize, bool isLastPacket);

bool VerifyFile(const char* filename, uint32_t expectedCRC);

#endif
