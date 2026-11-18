#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_PATH_LEN 256
#define SHM_NAME "Local\\Lab4_SharedMem"
#define SHM_SIZE 1024
#define EVT_WRITE "Local\\Lab4_EventWrite"
#define EVT_READ "Local\\Lab4_EventRead"

void PrintError(const char* msg) {
    DWORD errCode = GetLastError();
    printf("%s Error Code: %lu\n", msg, errCode);
}

int main (void){
    char filename[MAX_PATH_LEN];
    
    printf("Enter input file name: ");
    int a;
    if (scanf_s("%255s", filename, (unsigned)sizeof(filename)) != 1) {
        printf("Input error.\n");
        system("pause");
        scanf_s("%d", &a);
        return 1;
    }

    HANDLE hEventWrite = CreateEventA(NULL, FALSE, FALSE, EVT_WRITE);
    if (hEventWrite == NULL) {
        PrintError("Error: Failed to create EVT_WRITE event.");
        return 1;
    }

    HANDLE hEventRead = CreateEventA(NULL, FALSE, FALSE, EVT_READ);
    if (hEventRead == NULL) {
        PrintError("Error: Failed to create EVT_READ event.");
        CloseHandle(hEventWrite); // Освобождаем первое событие
        return 1;
    }

    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHM_SIZE, SHM_NAME);
    if (hMapFile == NULL) {
        PrintError("Error: Failed to create File Mapping object.");
        CloseHandle(hEventWrite);
        CloseHandle(hEventRead);
        return 1;
    }

    char* pBuf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
    if (pBuf == NULL) {
        PrintError("Error: Failed to map view of shared memory.");
        CloseHandle(hMapFile);
        CloseHandle(hEventWrite);
        CloseHandle(hEventRead);
        return 1;
    }

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.lpSecurityDescriptor = NULL;
    saAttr.bInheritHandle = TRUE;

    HANDLE InputFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, &saAttr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (InputFile == INVALID_HANDLE_VALUE) {
        PrintError("Can't open file");
        PrintError("Can't open file");
        system("pause");
        scanf_s("%d", &a);

        return 1;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = InputFile;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    BOOL success = CreateProcessA("child.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (!success) {
        PrintError("Cant start child.exe");
        CloseHandle(InputFile);
        return 1;
    }
    CloseHandle(InputFile);

    // Ждем либо запись в память от потомка либо завершение потомка
    HANDLE wait_objects[2] = { hEventWrite, pi.hProcess };

    while (1) {
        DWORD wait_res = WaitForMultipleObjects(2, wait_objects, FALSE, INFINITE);

        if (wait_res == WAIT_OBJECT_0) {
            printf("%s\n", pBuf);
            SetEvent(hEventRead); 
        }
        else {
            break;
        }
    }

    DWORD exitCode = 0;
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        if (exitCode == 2) {
            printf("\nChild process reported division by zero error.\n");
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hEventWrite);
    CloseHandle(hEventRead);

    printf("\nEnter for exit");
    getchar();
    getchar();
    return 0;
}