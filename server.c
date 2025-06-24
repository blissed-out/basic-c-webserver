#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    
    // create socket connection
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // configure the socket connection
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(8080); 
    server_addr.sin_addr.s_addr = INADDR_ANY; // bind to all available network interfaces (example: 127.0.0.1)
    
    // bind the socket to port
    
    return 0;
}