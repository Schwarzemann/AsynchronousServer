#include <winsock2.h>
#include <iostream>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 7777
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr{};
    char buffer[BUFFER_SIZE];
    int recvLen;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    // Create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Set up the server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost

    // Connect to the server
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server.\n";

    // Send a message to the server
    std::string message = "succ";
    if (send(clientSocket, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Message sent to server.\n";

    // Receive a response from the server
    recvLen = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (recvLen == SOCKET_ERROR) {
        std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (recvLen == 0) {
        std::cout << "Server closed the connection.\n";
    }
    else {
        std::cout << "Received from server: " << std::string(buffer, recvLen) << "\n";
    }

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
