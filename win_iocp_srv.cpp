#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 7777
#define BUFFER_SIZE 1024
#define TRUSTED_SIGNATURE "signature"

// Structure to hold per-client context
struct ClientContext {
    SOCKET clientSocket;
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[BUFFER_SIZE];
};

HANDLE completionPort;

void handleRequest(ClientContext* context, DWORD bytesTransferred) {
    std::string request(context->buffer, bytesTransferred);

    std::cout << "REQUEST: RECEIVED\n";  // Log when request is received

    // Check if the request contains a valid signature
    std::size_t pos = request.find("REQUEST:");
    if (pos != std::string::npos) {
        std::string signature = request.substr(pos + 8);  // Extract the signature

        std::cout << "REQUEST: SIGNATURE CHECK -> ";  // Start log for signature check

        if (signature == TRUSTED_SIGNATURE) {
            // Log signature approval
            std::cout << "APPROVED\n";

            // Approved
            std::string response = "APPROVED";
            send(context->clientSocket, response.c_str(), response.size(), 0);

            std::cout << "REQUEST: APPROVED\n";  // Log request approval
        }
        else {
            // Log incorrect signature
            std::cout << "INCORRECT\n";

            // Denied
            std::string response = "DENIED";
            send(context->clientSocket, response.c_str(), response.size(), 0);

            std::cout << "REQUEST: DENIED\n";  // Log request denial
        }
    }
}

void workerThread() {
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped;

    while (true) {
        BOOL result = GetQueuedCompletionStatus(completionPort, &bytesTransferred, &completionKey, &overlapped, INFINITE);

        if (result == FALSE || bytesTransferred == 0) {
            std::cerr << "Client disconnected or error occurred.\n";
            ClientContext* context = reinterpret_cast<ClientContext*>(completionKey);
            closesocket(context->clientSocket);
            delete context;  // Clean up the client context
            continue;
        }

        ClientContext* context = reinterpret_cast<ClientContext*>(completionKey);
        handleRequest(context, bytesTransferred);

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
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    std::vector<std::thread> workers;
    for (int i = 0; i < 2; ++i) {
        workers.push_back(std::thread(workerThread));
    }

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);

        ClientContext* context = new ClientContext();
        context->clientSocket = clientSocket;
        ZeroMemory(&context->overlapped, sizeof(OVERLAPPED));
        context->wsaBuf.len = BUFFER_SIZE;
        context->wsaBuf.buf = context->buffer;

        // Associate client context with the IO completion port
        CreateIoCompletionPort((HANDLE)clientSocket, completionPort, (ULONG_PTR)context, 0);

        // Start the first asynchronous receive
        DWORD flags = 0;
        WSARecv(clientSocket, &context->wsaBuf, 1, NULL, &flags, &context->overlapped, nullptr);
    }

    for (auto& thread : workers) {
        thread.join();
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
