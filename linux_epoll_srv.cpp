#include <sys/epoll.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <atomic>

#define PORT 7777
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

// Global variable to control server shutdown
std::atomic<bool> serverRunning(true);

// Function to make a socket non-blocking
int makeSocketNonBlocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

// Function to handle client data
void handleClientData(int client_sock, char* buffer, ssize_t count) {
    // Print received message
    std::cout << "Received: " << std::string(buffer, count) << "\n";

    // Echo the message back to the client
    ssize_t sent = write(client_sock, buffer, count);
    if (sent == -1) {
        perror("write");
    }
}

// Worker thread function
void* workerThread(void* arg) {
    int epoll_fd = *reinterpret_cast<int*>(arg);
    epoll_event events[MAX_EVENTS];

    while (serverRunning) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue; // Handle interrupts gracefully
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].events & EPOLLIN) {
                int client_sock = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                
                ssize_t count = read(client_sock, buffer, sizeof(buffer));
                if (count == -1) {
                    perror("read");
                    close(client_sock);
                } else if (count == 0) {
                    // Client disconnected
                    close(client_sock);
                } else {
                    // Process the received data
                    handleClientData(client_sock, buffer, count);
                }
            }
        }
    }

    // Cleanup when the thread finishes
    std::cout << "Worker thread exiting...\n";
    pthread_exit(nullptr);
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return -1;
    }

    if (listen(server_sock, SOMAXCONN) == -1) {
        perror("listen");
        return -1;
    }

    if (makeSocketNonBlocking(server_sock) == -1) {
        return -1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }

    epoll_event event{};
    event.data.fd = server_sock;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &event) == -1) {
        perror("epoll_ctl");
        return -1;
    }

    // Create worker threads
    std::vector<pthread_t> threads;
    int num_threads = 2; // Number of worker threads
    for (int i = 0; i < num_threads; ++i) {
        pthread_t thread;
        pthread_create(&thread, nullptr, workerThread, &epoll_fd);
        threads.push_back(thread);
    }

    // Main loop to accept incoming connections
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    while (serverRunning) {
        int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_len);
        
        if (client_sock == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No pending connections, this is expected in non-blocking mode
                continue;
            } else {
                perror("accept");
                break; // Some other error occurred, exit the loop or handle it
            }
        }

        makeSocketNonBlocking(client_sock);

        event.data.fd = client_sock;
        event.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &event) == -1) {
            perror("epoll_ctl");
            close(client_sock);
        }
    }

    // Shutdown procedures
    serverRunning = false;
    for (pthread_t& thread : threads) {
        pthread_join(thread, nullptr);
    }

    close(server_sock);
    close(epoll_fd);
    return 0;
}
