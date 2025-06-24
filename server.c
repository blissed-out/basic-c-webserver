#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  printf("main function called\n");
  int server_fd;
  struct sockaddr_in server_addr;

  // create socket connection
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  // configure the socket connection
  server_addr.sin_family = AF_INET; // IPv4
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY; // bind to all available network
                                            // interfaces (example: 127.0.0.1)

  // bind the socket to port
  int bind_resposne =
      bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  // listen for the connection
  listen(server_fd, 2);

  printf("Server is running on port 8080\n");

  // accept client connection
  while (1) {

    int client_fd;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    // recieve data from client
    char buffer[7 * 1024 * 1024]; // 7 mb
    ssize_t recv_response = recv(client_fd, buffer, sizeof(buffer), 0);

    FILE *html = fopen("index.html", "r");
    fseek(html, 0, SEEK_END);
    long filesize = ftell(html);
    rewind(html);
    
    char *body = malloc(filesize);
    fread(body, filesize, 1, html);
    fclose(html);
    
    char header[256];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "Content-Type: text/html\r\n"
             "\r\n", filesize);
    
    send(client_fd, header, strlen(header), 0);
    send(client_fd, body, strlen(body), 0);
    close(client_fd);
  }

  close(server_fd);

  return 0;
}
