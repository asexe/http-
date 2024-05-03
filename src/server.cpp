#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
std::cout << "Logs from your program will appear here!\n";
std::string report;

// Uncomment this block to pass the first stage
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd < 0) {
std::cerr << "Failed to create server socket\n";
return 1;
}
//
// // Since the tester restarts your program quite often, setting REUSE_PORT
// // ensures that we don't run into 'Address already in use' errors
int reuse = 1;
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
std::cerr << "setsockopt failed\n";
return 1;
}

struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;
server_addr.sin_port = htons(4221);
//
if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
std::cerr << "Failed to bind to port 4221\n";
return 1;
}
//
int connection_backlog = 5;
if (listen(server_fd, connection_backlog) != 0) {
std::cerr << "listen failed\n";
return 1;
}
//
struct sockaddr_in client_addr;
int client_addr_len = sizeof(client_addr);

std::cout << "Waiting for a client to connect...\n";

int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
// Receive the HTTP request from the client
char buffer[1024];
int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
if (bytes_received < 0) {
    std::cerr << "Error receiving data from client\n";
    close(client_fd);
    return 1;
}

// Parse the start line to extract the method, path, and HTTP version
std::string request(buffer, bytes_received);
size_t start_pos = request.find(" ");
size_t end_pos = request.find(" ", start_pos + 1);
if (start_pos != std::string::npos && end_pos != std::string::npos) {
    std::string method = request.substr(0, start_pos);
    std::string path = request.substr(start_pos + 1, end_pos - start_pos - 1);

    // Print the extracted path for debugging
    std::cout << "Received path: " << path << std::endl;

    // Check if the path is "/"
    if (path == "/index.html") {
        // Respond with a 200 OK response
        report = "HTTP/1.1 200 OK\r\n\r\n";
    } else {
        // Respond with a 404 Not Found response
        report = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
} else {
    // Invalid request format
    report = "HTTP/1.1 400 Bad Request\r\n\r\n";
}


// Send the response to the client
send(client_fd, report.c_str(), report.length(), 0);
close(server_fd);

  return 0;
}