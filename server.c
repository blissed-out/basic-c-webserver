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
    int bind_resposne = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    // listen for the connection
    listen(server_fd, 2);
    
    printf("Server is running on port 8080");
    
    // accept client connection
    int client_fd;
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    accept(client_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    
    
    
    
    return 0;
}