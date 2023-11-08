#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE 512
#define DEFAULT_PORT "51488"

DWORD WINAPI PipeReader(LPVOID);
DWORD WINAPI PipeWriter(LPVOID);

SOCKET ConnectionSocket = INVALID_SOCKET;

int main(int argc, char** argv) {
    WSADATA wsaData;

    if (argc != 2) {
        printf("Usage: %s <server-address>\n", argv[0]);
        return 1;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    struct addrinfo* result = NULL, * ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        ConnectionSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectionSocket == INVALID_SOCKET) {
            printf("Socket creation failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        iResult = connect(ConnectionSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectionSocket);
            ConnectionSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectionSocket == INVALID_SOCKET) {
        printf("Unable to connect to the server!\n");
        WSACleanup();
        return 1;
    }

    HANDLE pipeThreads[2];
    pipeThreads[0] = CreateThread(NULL, 0, PipeWriter, NULL, 0, NULL);
    pipeThreads[1] = CreateThread(NULL, 0, PipeReader, NULL, 0, NULL);

    WaitForMultipleObjects(2, pipeThreads, TRUE, INFINITE);

    iResult = shutdown(ConnectionSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("Shutdown failed with error: %d\n", WSAGetLastError());
    }

    closesocket(ConnectionSocket);
    WSACleanup();

    return 0;
}

DWORD WINAPI PipeWriter(LPVOID dummy) {
    UNREFERENCED_PARAMETER(dummy);
    DWORD iResult, dwRead, bSuccess;

    for (;;) {
        CHAR buffer[BUFSIZE] = { 0 };
        bSuccess = ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, BUFSIZE - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) {
            return -1;
        }
        buffer[dwRead] = '\0';

        iResult = send(ConnectionSocket, buffer, dwRead, 0);
        if (iResult == SOCKET_ERROR) {
            printf("Send failed with error: %d\n", WSAGetLastError());
            return -1;
        }
    }
}

DWORD WINAPI PipeReader(LPVOID dummy) {
    UNREFERENCED_PARAMETER(dummy);
    int iResult, bSuccess;
    char buffer[BUFSIZE];

    for (;;) {
        iResult = recv(ConnectionSocket, buffer, BUFSIZE - 1, 0);
        if (iResult > 0) {
            buffer[iResult] = '\0';
            bSuccess = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, iResult, NULL, NULL);
            if (!bSuccess) {
                return -1;
            }
        }
        else {
            return -1;
        }
    }
}
