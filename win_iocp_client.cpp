#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 7777
#define BUFFER_SIZE 1024
#define SIGNATURE "signature"  // This will be automated later, but now this'll do

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Assuming server is running locally

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to the server. Press Enter to send a request...\n";
    std::cin.get();  // Wait for the user to press Enter

    // Send request to server with the client's "signature"
    std::string request = "REQUEST:" + std::string(SIGNATURE);
    send(clientSocket, request.c_str(), request.size(), 0);

    // Receive response from server
    char buffer[BUFFER_SIZE] = { 0 };
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived > 0) {
        std::string response(buffer, bytesReceived);
        std::cout << "Server Response: " << response << "\n";

        if (response.find("APPROVED") != std::string::npos) {
            std::cout << "Connection approved. You can now send messages.\n";

            // Start sending messages
            while (true) {
                std::string message;
                std::cout << "Enter message: ";
                std::getline(std::cin, message);  // Take user input

                if (message == "exit") {
                    break;
                }

                // Send message to the server
                send(clientSocket, message.c_str(), message.size(), 0);
            }
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
