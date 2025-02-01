#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H
#include <stdbool.h>

int loadFile(const char* filename, char** buffer, size_t* size);
int saveFile(const char* filename, const char* buffer, size_t size);

int SendFile(void);

int ReceiveFile(void);

int VerifyFile(void);

#endif
