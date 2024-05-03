#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

std::string captureAfterEcho(const std::string& input) {
    std::size_t echoPos = input.find("/echo/");
    if (echoPos == std::string::npos) {
        // 如果没有找到 /echo/，返回空字符串
        return "";
    }
    // 从 /echo/ 后面开始查找空格
    std::size_t spacePos = input.find(' ', echoPos + 6); // /echo/ 长度为6
    if (spacePos == std::string::npos) {
        // 如果没有找到空格，取从 /echo/ 后面到字符串末尾的部分
        return input.substr(echoPos + 6);
    } else {
        // 如果找到了空格，取空格前的部分
        return input.substr(echoPos + 6, spacePos - echoPos - 6);
    }
}

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
    std::cout << path << std::endl;
    // Print the extracted path for debugging
    /*std::cout << "Received path: " << path << std::endl;*/

    // Check if the path include "/" or include "/echo/"
    if (captureAfterEcho(path) != ""|| path == "/"|| path == "/index.html") {
        // Respond with a 200 OK response
        report = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(captureAfterEcho(path).length()) + "\r\n\r\n" + captureAfterEcho(path) ;
        std::cout << "Content-Type: text/plain" << std::endl;
        std::cout << "Content-Length: "<< captureAfterEcho(path).length() << std::endl;
        std::cout << captureAfterEcho(path) << std::endl;
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