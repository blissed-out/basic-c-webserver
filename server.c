#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const char* get_content_type(const char *filename) {
    // find the last '.' in the filename
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";  // default binary

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".txt") == 0) return "text/plain";

    return "application/octet-stream";
}

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

    buffer[recv_response] = '\0';

    char method[10], path[999];
    sscanf(buffer, "%s %s", method, path); // GET /index.html

    char full_path[1024];
    if (strcmp(path, "/") == 0) {
        strcpy(full_path, "index.html");
    } else {
        snprintf(full_path, sizeof(full_path), "%s", path + 1); // index.html
    }

    FILE *file = fopen(full_path, "r");

    // send 404 response if file not found
    if (file == NULL) {
      perror("file open failed");
      char failure_message[] = "<html><body><h1>404 Not Found</h1></body></html>";
      char buffer[999];

      snprintf(buffer, sizeof(buffer),
               "HTTP/1.1 404 Not Found\r\n"
               "Content-Length: %lu\r\n"
               "Content-Type: text/html\r\n"
               "\r\n"
               , strlen(failure_message));

      send(client_fd, buffer, strlen(buffer), 0);
      send(client_fd, failure_message, strlen(failure_message), 0);
      close(client_fd);
      continue;
   }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    char *body = malloc(filesize);
    fread(body, filesize, 1, file);
    fclose(file);

    char header[999];
    const char *content_type = get_content_type(full_path);

    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             filesize,
             content_type);
    
    printf("this is header: %s", header);

    send(client_fd, header, strlen(header), 0);
    send(client_fd, body, strlen(body), 0);
    free(body);
    close(client_fd);
  }

  close(server_fd);

  return 0;
}
