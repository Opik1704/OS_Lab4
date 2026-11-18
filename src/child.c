#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>


#define MAX_LINE 1024
#define SHM_NAME "Local\\Lab4_SharedMem"
#define SHM_SIZE 1024
#define EVT_WRITE "Local\\Lab4_EventWrite"
#define EVT_READ "Local\\Lab4_EventRead"

int main(void) {
    HANDLE hEventWrite = OpenEventA(EVENT_MODIFY_STATE, FALSE, EVT_WRITE);
    if (hEventWrite == NULL) {
        fprintf(stderr, "[Child] Error: Failed to open EVT_WRITE event. Error code: %lu\n", GetLastError());
        return 1;
    }

    HANDLE hEventRead = OpenEventA(SYNCHRONIZE, FALSE, EVT_READ);
    if (hEventRead == NULL) {
        fprintf(stderr, "[Child] Error: Failed to open EVT_READ event. Error code: %lu\n", GetLastError());
        CloseHandle(hEventWrite);
        return 1;
    }

    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
    if (hMapFile == NULL) {
        fprintf(stderr, "[Child] Error: Failed to open shared memory mapping. Error code: %lu\n", GetLastError());
        CloseHandle(hEventWrite);
        CloseHandle(hEventRead);
        return 1;
    }

    char* pBuf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);

    if (pBuf == NULL) {
        fprintf(stderr, "[Child] Error: Failed to map view of shared memory. Error code: %lu\n", GetLastError());
        CloseHandle(hMapFile);
        CloseHandle(hEventWrite);
        CloseHandle(hEventRead);
        return 1;
    }

    char line[MAX_LINE];
    
    while (fgets(line, sizeof(line), stdin) != NULL) {
        char* context = NULL;
        char* token = strtok_s(line, " \t\r\n", &context);
        if (token == NULL) {
            continue; 
        }

        int result = atoi(token);
        int div_by_zero = 0;
        while ((token = strtok_s(NULL, " \t\r\n", &context)) != NULL) {
            int num = atoi(token);
            if (num == 0) {
                div_by_zero = 1;
                break;
            }
            result = result / num;
        }
        if (div_by_zero) {
            fprintf(stderr, "[Child] Error: Division by zero detected!\n");
            fflush(stderr);

            UnmapViewOfFile(pBuf);
            CloseHandle(hMapFile);
            CloseHandle(hEventWrite);
            CloseHandle(hEventRead);
            exit(2);
        }

        sprintf_s(pBuf, SHM_SIZE, "%d", result);
        // Будим родителя: данные готовы
        SetEvent(hEventWrite);
        // Ждем, пока родитель прочитает и выведет на экран
        WaitForSingleObject(hEventRead, INFINITE);

    }
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hEventWrite);
    CloseHandle(hEventRead);
    return 0;

}