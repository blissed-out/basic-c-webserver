#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    printf("main function called\n");
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

    printf("Server is running on port 8080\n");


    // accept client connection
    int client_fd;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    // recieve data from client
    char buffer[7 * 1024 * 1024];
    ssize_t recv_response = recv(client_fd, buffer, sizeof(buffer), 0);

    // char method[10], path[1024];
    // sscanf(buffer, "%s %s", method, path);
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 14\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello Universe";

    send(client_fd, response, strlen(response), 0);

    close(client_fd);
    close(server_fd);

    return 0;
}
