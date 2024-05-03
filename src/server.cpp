#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

// Function to parse the request and extract the path
std::string extractPath(const std::string& request) {
    std::string path;
    size_t startPos = request.find("GET /");
    if (startPos != std::string::npos) {
        size_t endPos = request.find(" ", startPos + 5);
        if (endPos != std::string::npos) {
            path = request.substr(startPos + 5, endPos - startPos - 5);
        }
    }
    return path;
}

int main(int argc, char **argv) {
    std::cout << "Logs from your program will appear here!\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    std::cout << "Waiting for a client to connect...\n";

    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_fd < 0) {
        std::cout << "Connect Failed\n";
    } else {
        std::cout << "Client connected\n";

        char buffer[1024];
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0'; // Null-terminate the string
            std::string request(buffer);

            std::string path = extractPath(request);
            // Check if the path is root or index.html
            if (path == "/" || path == "/index.html") {
                std::string response = "HTTP/1.1 200 OK\r\n\r\n";
                send(client_fd, response.c_str(), response.length(), 0);
            } else {
                std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
                send(client_fd, response.c_str(), response.length(), 0);
            }
        }
    }

    close(server_fd);

    return 0;
}