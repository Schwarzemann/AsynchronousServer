#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 7777
#define BUFFER_SIZE 1024

// Structure to hold per-client context
struct ClientContext {
    SOCKET clientSocket;
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[BUFFER_SIZE];
};

// Global completion port handle
HANDLE completionPort;

// Function for worker threads to process I/O completion packets
void workerThread() {
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped;

    while (true) {
        BOOL result = GetQueuedCompletionStatus(completionPort, &bytesTransferred, &completionKey, &overlapped, INFINITE);

        if (!result) {
            std::cerr << "GetQueuedCompletionStatus failed: " << GetLastError() << "\n";
            continue;
        }

        if (bytesTransferred == 0) {
            // Client disconnected or error occurred
            std::cerr << "Client disconnected or error occurred.\n";
            SOCKET clientSocket = static_cast<SOCKET>(completionKey);
            closesocket(clientSocket);
            continue;
        }

        ClientContext* context = reinterpret_cast<ClientContext*>(overlapped);
        std::cout << "Received: " << std::string(context->buffer, bytesTransferred) << "\n";

        // Echo back to client
        WSABUF wsaBuf = { bytesTransferred, context->buffer };
        DWORD sentBytes = 0;
        WSASend(context->clientSocket, &wsaBuf, 1, &sentBytes, 0, &context->overlapped, nullptr);

        // Prepare for the next receive
        ZeroMemory(&context->overlapped, sizeof(OVERLAPPED));
        context->wsaBuf.len = BUFFER_SIZE;
        context->wsaBuf.buf = context->buffer;

        DWORD flags = 0;
        WSARecv(context->clientSocket, &context->wsaBuf, 1, NULL, &flags, &context->overlapped, nullptr);
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << "\n";
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (completionPort == NULL) {
        std::cerr << "CreateIoCompletionPort failed: " << GetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::vector<std::thread> workers;
    int num_threads = 2; // Number of worker threads
    for (int i = 0; i < num_threads; ++i) {
        workers.push_back(std::thread(workerThread));
    }

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
            continue;
        }

        ClientContext* context = new ClientContext();
        context->clientSocket = clientSocket;
        ZeroMemory(&context->overlapped, sizeof(OVERLAPPED));
        context->wsaBuf.len = BUFFER_SIZE;
        context->wsaBuf.buf = context->buffer;

        if (CreateIoCompletionPort((HANDLE)clientSocket, completionPort, (ULONG_PTR)clientSocket, 0) == NULL) {
            std::cerr << "CreateIoCompletionPort failed: " << GetLastError() << "\n";
            closesocket(clientSocket);
            delete context;
            continue;
        }

        // Start the first asynchronous receive
        DWORD flags = 0;
        if (WSARecv(clientSocket, &context->wsaBuf, 1, NULL, &flags, &context->overlapped, nullptr) == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSA_IO_PENDING) {
                std::cerr << "WSARecv failed: " << error << "\n";
                closesocket(clientSocket);
                delete context;
            }
        }
    }

    // Shutdown procedures
    for (auto& thread : workers) {
        thread.join();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
