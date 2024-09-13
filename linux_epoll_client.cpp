/* 
    Still in progress so don't fuck with it. 
    FYI - Windows client is on its way. 
*/

#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Socket creation error\n";
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7777);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return -1;
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Connection failed\n";
        return -1;
    }

    std::string message = "Hello, server!";
    send(sock, message.c_str(), message.length(), 0);

    char buffer[1024] = {0};
    int bytesReceived = read(sock, buffer, 1024);
    std::cout << "Server: " << std::string(buffer, bytesReceived) << "\n";

    close(sock);
    return 0;
}
