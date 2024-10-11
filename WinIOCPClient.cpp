#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <iomanip>  // For formatting table

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_SIZE 1024
#define SIGNATURE "signature"  // This will be automated later, but now this'll do
#define SCAN_START_PORT 7700    // Start of the port scan range
#define SCAN_END_PORT 7800      // End of the port scan range

// Function to check if a port is open
bool isPortOpen(const std::string& address, int port) {
    SOCKET tempSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tempSocket == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(address.c_str());

    bool open = (connect(tempSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR);
    closesocket(tempSocket);

    return open;
}

// Function to scan a range of ports and display open ports in a table
void scanPorts(const std::string& address) {
    std::cout << "Scanning ports " << SCAN_START_PORT << " to " << SCAN_END_PORT << "...\n";
    std::cout << std::setw(10) << "Port" << std::setw(15) << "Status" << "\n";
    std::cout << "-------------------------\n";

    for (int port = SCAN_START_PORT; port <= SCAN_END_PORT; ++port) {
        bool open = isPortOpen(address, port);
        std::cout << std::setw(10) << port << std::setw(15) << (open ? "OPEN" : "CLOSED") << "\n";
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    std::string serverAddress = "127.0.0.1";  // Assuming server is running locally
    int selectedPort = 0;

    // Perform a port scan and show the results
    scanPorts(serverAddress);

    // Prompt the user to select a port
    std::cout << "\nEnter the port number to connect to the server (or 0 to manually input): ";
    std::cin >> selectedPort;

    if (selectedPort == 0) {
        // Manually input a port if not found
        std::cout << "Manually enter the port number: ";
        std::cin >> selectedPort;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(selectedPort);  // Use the selected port
    serverAddr.sin_addr.s_addr = inet_addr(serverAddress.c_str());

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed on port " << selectedPort << ".\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Prompt the user for a username
    std::string username;
    std::cout << "Enter your username: ";
    std::cin.ignore();  // Clear the newline left in the input buffer
    std::getline(std::cin, username);

    // Send the username to the server
    std::string usernameRequest = "USERNAME:" + username;
    send(clientSocket, usernameRequest.c_str(), usernameRequest.size(), 0);

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
