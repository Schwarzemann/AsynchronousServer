#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <map>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 7777
#define BUFFER_SIZE 1024
#define TRUSTED_SIGNATURE "signature"

std::map<SOCKET, int> clientIds;
std::map<SOCKET, std::string> clientUsernames; // Map to store usernames
int nextClientId = 1;

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

    // Check if the request is for the username
    std::size_t pos = request.find("USERNAME:");
    if (pos != std::string::npos) {
        std::string username = request.substr(pos + 9);  // Extract the username
        std::cout << "Username received: " << username << "\n";

        // Assign client an ID and store the username
        int clientId = nextClientId++;
        clientIds[context->clientSocket] = clientId;
        clientUsernames[context->clientSocket] = username;

        // Send approval message to the client
        std::string response = "APPROVED. Welcome, " + username + " (ID: " + std::to_string(clientId) + ")";
        send(context->clientSocket, response.c_str(), response.size(), 0);

        std::cout << "Username " << username << " assigned with Client ID: " << clientId << "\n";  // Log the assignment
    }
    else {
        // Treat it as a message from the client
        int clientId = clientIds[context->clientSocket];
        std::string username = clientUsernames[context->clientSocket];
        std::string message(context->buffer, bytesTransferred);

        std::cout << "Client " << clientId << " (" << username << ") says: " << message << "\n";
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
