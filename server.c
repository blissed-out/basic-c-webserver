#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 100 * 1024 * 1024 // 100 mb

const char *get_content_type(const char *filename) {

  // find the last '.' in the filename
  const char *ext = strrchr(filename, '.');

  if (!ext)
    return "application/octet-stream"; // default binary

  if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
    return "text/html";

  if (strcmp(ext, ".css") == 0)
    return "text/css";

  if (strcmp(ext, ".js") == 0)
    return "application/javascript";

  if (strcmp(ext, ".json") == 0)
    return "application/json";

  if (strcmp(ext, ".png") == 0)
    return "image/png";

  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    return "image/jpeg";

  if (strcmp(ext, ".gif") == 0)
    return "image/gif";

  if (strcmp(ext, ".svg") == 0)
    return "image/svg+xml";

  if (strcmp(ext, ".txt") == 0)
    return "text/plain";

  if (strcmp(ext, ".mp4"))
    return "video/mp4";

  if (strcmp(ext, ".webp"))
    return "image/webp";

  return "application/octet-stream";
}

void handle_client(int server_fd) {
  // infinite loop to keep listening client connections
  while (1) {

    int client_fd;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }

    // recieve data from client
    // dynamically allocate 100 mb memory
    char *buffer = malloc(BUFFER_SIZE); // 100 mb

    if (!buffer) {
      perror("malloc failed for buffer");
      close(client_fd);
      continue;
    }

    ssize_t recv_response = recv(client_fd, buffer, BUFFER_SIZE, 0);

    if (recv_response < 0) {
      perror("recieve data from client failed");
      close(client_fd);
      continue;
    }

    if (recv_response == 0) {
      printf("Client disconnected\n");
      close(client_fd);
      continue;
    }

    buffer[recv_response] = '\0'; // null terminate the string

    char method[10], path[999];
    sscanf(buffer, "%s %s", method, path);

    // check if method is GET
    if (strcmp(method, "GET") != 0) {

      printf("Your method is not GET, it's %s", method);
      perror("invalid method");

      close(client_fd);

      continue;
    }

    char full_path[1024];

    if (strcmp(path, "/") == 0) {
      strcpy(full_path, "index.html");
    } else {
      snprintf(full_path, sizeof(full_path), "%s", path + 1); // remove '/'
    }

    FILE *file = fopen(full_path, "rb"); // rb => read binary files

    // send 404 response if file not found
    if (file == NULL) {
      printf("Tried to get %s\n", full_path);
      perror("file open failed");

      char failure_message[] =
          "<html><body><h1>404 Not Found</h1></body></html>";

      char header[999];

      snprintf(header, sizeof(header),
               "HTTP/1.1 404 Not Found\r\n"
               "Content-Length: %lu\r\n"
               "Content-Type: text/html\r\n"
               "\r\n",
               strlen(failure_message));

      int header_send_response = send(client_fd, header, strlen(header), 0);

      if (header_send_response < 0) {
        perror("header send failed");
      }

      int failure_message_send_response =
          send(client_fd, failure_message, strlen(failure_message), 0);

      if (failure_message_send_response < 0) {
        perror("failure message send failed");
      }

      close(
          client_fd); // close the client connection after sending the response

      continue;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    char *body = malloc(filesize);

    if (!body) {
      perror("malloc failed for body");

      close(client_fd);

      continue;
    }

    // size_t bytes_read = fread(body, filesize, 1, file);
    size_t bytes_read = fread(body, 1, filesize, file);

    if (bytes_read != filesize) {
      perror("fread failed");

      free(body);
      close(client_fd);

      continue;
    }

    fclose(file);

    // send body
    char header[999];
    const char *content_type = get_content_type(full_path); // get content type

    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             filesize, content_type);

    printf("this is header: %s", header);

    int header_response = send(client_fd, header, strlen(header), 0);

    if (header_response < 0) {
      perror("header send failed");
    }

    ssize_t body_response = send(client_fd, body, filesize, 0);

    if (body_response < 0) {
      perror("body send failed");
    }

    free(body);

    free(buffer);

    close(client_fd);
  }
}

int main() {
  printf("main function called\n");
  int server_fd;
  struct sockaddr_in server_addr;

  // create socket connection
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // configure the socket connection
  server_addr.sin_family = AF_INET; // IPv4
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY; // bind to all available network
                                            // interfaces (example: 127.0.0.1)

  // bind the socket to port
  int bind_response =
      bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  if (bind_response) {
    printf("PORT NUMBER: %d\n", PORT);
    perror("could not bind socket to the port");
    exit(EXIT_FAILURE);
  }

  // listen for the connection

  if (listen(server_fd, 2)) {
    perror("could not listen to the port");
    exit(EXIT_FAILURE);
  }

  printf("Server is running on port %d\n", PORT);

  // accept client connection
  handle_client(server_fd);

  close(server_fd);

  return 0;
}
