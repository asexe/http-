#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>

std::string captureAfterKey(const std::string& input) {
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

std::string extractUserAgent(const std::string& request) {
    std::size_t userAgentPos = request.find("User-Agent: ");
    if (userAgentPos == std::string::npos) {
        // 如果没有找到 User-Agent 头，返回空字符串
        return "";
    }
    // 找到 User-Agent 头，现在找到该行的结束位置
    std::size_t endOfLinePos = request.find("\r\n", userAgentPos);
    if (endOfLinePos == std::string::npos) {
        // 如果没有找到行结束，返回空字符串
        return "";
    }
    // 提取 User-Agent 头的值
    return request.substr(userAgentPos + sizeof("User-Agent: ") - 1, endOfLinePos - userAgentPos - sizeof("User-Agent: ") + 1);
}


int matchPath(const char *keyword[], int keywordSize, const std::string& path) {
    for (int i = 0; i < keywordSize; ++i) {
        if (std::strcmp(keyword[i], path.c_str()) == 0) {
            return 1; // 找到匹配，返回 1
        }
    }
    return 0; // 没有找到匹配，返回 0
}

int matchEcho(const std::string& path, const std::vector<std::string>& array) {
    for (const auto& str : array) {
        if (path.find(str) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// 新增函数，用于读取文件内容并返回
std::string readFileContent(const std::string& filePath) {
    std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
    if (fileStream) {
        std::streamsize size = fileStream.tellg();
        fileStream.seekg(0, std::ios::beg);

        std::string content;
        content.resize(size);
        if (size > 0) {
            fileStream.read(&content[0], size);
        }
        return content;
    } else {
        return "";
    }
}

// 新增函数，用于读取文件内容并返回
std::string readFileContent(const std::string& directory, const std::string& filename) {
    std::ifstream fileStream((directory + "/" + filename).c_str(), std::ios::binary | std::ios::ate);
    if (fileStream) {
        std::streamsize size = fileStream.tellg();
        fileStream.seekg(0, std::ios::beg);

        std::string content((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        return content;
    } else {
        return "";
    }
}

// 新建函数 processRequest 来处理请求
std::string processRequest(const std::string& request, const std::string& directory, const std::vector<std::string>& keyword) {
    std::string report;
    size_t start_pos = request.find(" ");
    size_t end_pos = request.find(" ", start_pos + 1);
    
    if (start_pos != std::string::npos && end_pos != std::string::npos) {
        std::string method = request.substr(0, start_pos);
        std::string path = request.substr(start_pos + 1, end_pos - start_pos - 1);
        std::cout << "Received path: " << path << std::endl;
        
        // 提取 User-Agent 头的值
        std::string userAgent = extractUserAgent(request);
        
        if (path == "/") {
            report = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
        }
        // 处理 /user-agent 请求
        else if (path == "/user-agent") {
            report = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " 
                     + std::to_string(userAgent.length()) + "\r\n\r\n" + userAgent;
        }
        // 处理 /echo/ 请求
        else if (path.find("/echo/") == 0) {
            std::string responseContent = captureAfterKey(request);
            report = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " 
                     + std::to_string(responseContent.length()) + "\r\n\r\n" + responseContent;
        }
        // 处理其他请求，需要directory参数
        else {
            if (directory.empty()) {
                report = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
                return report;
            }
            
            if (path.find("/files/") == 0) {
                // 提取文件名
                std::string filename = path.substr(7); // 去掉 "/files/" 前缀
                std::string responseContent = readFileContent(directory, filename);
                
                // 如果文件内容不为空，设置正确的响应类型
                if (!responseContent.empty()) {
                    report = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " 
                             + std::to_string(responseContent.length()) + "\r\n\r\n" + responseContent;
                } else {
                    report = "HTTP/1.1 404 Not Found\r\n\r\n";
                }
            } else {
                report = "HTTP/1.1 404 Not Found\r\n\r\n";
            }
        }
    } else {
        report = "HTTP/1.1 400 Bad Request\r\n\r\n";
    }
    
    return report;
}

void handle_client(int client_fd, struct sockaddr_in client_addr, const std::string& directory) {
    char buffer[1024];
    std::string report;
    std::vector<std::string> keyword = {"/files/", "/echo/", "/index.html", "/user-agent"};
    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) {
        std::cerr << "Error receiving data from client\n";
        close(client_fd);
        return;
    }

    std::string request(buffer, bytes_received);
    report = processRequest(request, directory, keyword);

    // Send the response to the client
    send(client_fd, report.c_str(), report.length(), 0);
    close(client_fd);
}

int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
std::cout << "Logs from your program will appear here!\n";
std::string directory;
for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--directory" && i + 1 < argc) {
        directory = argv[++i];
        break;
    }
}

if (directory.empty()) {
    std::cerr << "Error: No directory specified with --directory flag.\n";
}


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

    while (true) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        if (client_fd < 0) {
            std::cerr << "Error accepting connection\n";
            continue; // Skip to the next iteration if accept fails
        }

        // Create a new thread to handle the client
        std::thread client_thread(handle_client, client_fd, client_addr, directory);
        client_thread.detach(); // Detach the thread to let it run independently
    }

    // Close the server socket when done (not reached in this example)
    close(server_fd);

    return 0;
}