#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H
#include <stdbool.h>
#include <stdint.h>

#define PACKET_SIZE 1024
#define CHECKSUM_SIZE 4  // CRC32 checksum size

int loadFile(const char* filename, char** buffer, size_t* size);
int saveFile(const char* filename, const char* buffer, size_t size);
double calculateTransferSpeed(double startTime, double endTime, size_t fileSize);

int SendFile(const char* filename, const char* destIP, int destPort);

int ReceiveFile(const char* outputFilename, int listenPort);

bool VerifyFile(const char* filename, uint32_t expectedCRC);

#endif
