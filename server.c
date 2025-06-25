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
    // default binary
    return "application/octet-stream";

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

  // main reason why I need to dynamically allocate buffer
  if (strcmp(ext, ".mp4") == 0)
    return "video/mp4";

  if (strcmp(ext, ".webp") == 0)
    return "image/webp";

  // incase we something random files are being served
  return "application/octet-stream";
}

void handle_client(int server_fd) {
  // infinite loop to keep listening client connections
  while (1) {

    // to store client file descriptor
    int client_fd;

    // to store client address information
    // https://beej.us/guide/bgnet/  - learned from Beej's Guide to Network
    // Programming
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // concept learned from Beej's Guide to Network Programming
    client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd < 0) {
      perror("accept failed");

      close(client_fd);

      continue;
    }

    // recieve data from client

    // dynamically allocate 100 mb memory
    char *buffer = malloc(BUFFER_SIZE);

    // checking if buffer is allocated successfully
    if (!buffer) {
      perror("malloc failed for buffer");

      // user should connect again
      close(client_fd);

      continue;
    }

    /* tried using int, but it execeed it's limit. So, using ssize_t (which
     seems to be better to store bytesize in, rather than int)
     */
    ssize_t recv_response = recv(client_fd, buffer, BUFFER_SIZE, 0);
    /* if successful, recv() returns the length of the message or datagram in
     bytes.
     */

    // checking if data is received successfully
    if (recv_response < 0) {
      perror("recieve data from client failed");

      close(client_fd);

      continue;
    }

    if (recv_response == 0) {
      printf("Client disconnected\n");

      /* buffer is dynamically allocated, so freeing it to prevent memory leaks,
      in case of client disconnection
      */
      free(buffer);

      // just making sure, connection is closed
      close(client_fd);

      continue;
    }

    // making sure buffer actually have null termination (string)
    // tried without null termination, got weird error
    buffer[recv_response] = '\0';
    // learned from here -
    // https://stackoverflow.com/questions/66430071/does-sscanf-require-a-null-terminated-string-as-input

    // extracting method and path from buffer
    char method[10], path[999];
    sscanf(buffer, "%s %s", method,
           path); // string manipulation, that's why we null temrinated above

    // checking if the request is GET, reject otherwise
    if (strcmp(method, "GET") != 0) {

      printf("Your method is not GET, it's %s", method);
      perror("invalid method");

      // if other than GET request, we close the client connection
      close(client_fd);

      continue;
    }

    // to store correct path which will remove '/'
    char correct_path[1024];
    // browser didn't load the file without removing '/' for some reason

    // generally web servers redirect root ('/') to index.html
    // learned from - web dev
    if (strcmp(path, "/") == 0) {
      // tried /index.html, browser didn't get it.
      strcpy(correct_path, "index.html");
    } else {
      // path + 1 means, remove the '/' from the path.
      snprintf(correct_path, sizeof(correct_path), "%s", path + 1);
    }

    // rb => read binary files
    FILE *file = fopen(correct_path, "rb"); // r only worked for text files
    /* tried using r, but it fails for files like:
    .png, .jpg, .jpeg, .gif, .svg, .ico, .mp4
     */

    // send 404 response if file not found
    if (file == NULL) {

      // just for debugging file that failed
      printf("Tried to get %s\n", correct_path);

      perror("file open failed");

      // sending 404 response in html format
      char failure_message[] =
          "<html><body><h1>404 Not Found</h1></body></html>";

      // browser needs header, otherwise it will not accept the response.
      char header[999];
      /* https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers -
       learned from here
       */

      /* snprintf - basically to format and store a series of characters and
       values into a character array
       learned from - https://www.geeksforgeeks.org/snprintf-c-library/
       */
      snprintf(header, sizeof(header),
               "HTTP/1.1 404 Not Found\r\n"
               "Content-Length: %lu\r\n"
               "Content-Type: text/html\r\n"
               "\r\n",
               strlen(failure_message));

      // capture response of header sent
      int header_send_response = send(client_fd, header, strlen(header), 0);

      if (header_send_response < 0) {
        perror("header send failed");

        // no point in holding connection if header send failed
        close(client_fd);
      }

      // just making sure, failure response is sent to client
      int failure_message_send_response =
          send(client_fd, failure_message, strlen(failure_message), 0);

      if (failure_message_send_response < 0) {
        perror("failure message send failed");

        // client should reconnect
        close(client_fd);
      }

      // close the client connection after sending the response
      close(client_fd);

      continue;
    }

    // basically reading file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    /* tried static allocation, but it failed in heavy web pages and got
    segmentation fault. So dynamically allocating memory for body to prevent
     stack overflow
     */
    char *body = malloc(filesize);

    if (!body) {
      perror("malloc failed for body");

      // in case system fails to allocate memory, we close the client connection
      close(client_fd);

      continue;
    }

    size_t bytes_read = fread(body, 1, filesize, file);

    /* making sure the bytes read are equal to the file size
    initially without, cutoff content was sent.
    just better user experience
    */
    if (bytes_read != filesize) {
      perror("fread failed");

      free(body);
      close(client_fd);

      continue;
    }

    // close the file after successful read
    fclose(file);

    // send body
    char header[999];
    // to dynamically set content type, because we are serving multiple types of
    // files
    const char *content_type = get_content_type(correct_path);

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

    // free memory of both body and buffer after client request is served
    free(body);
    free(buffer);

    // then close the client socket connection
    close(client_fd);
  }
}

int main() {
  printf("main function called\n");

  // server file descriptor
  int server_fd;

  // to store server address information
  struct sockaddr_in server_addr;

  // create socket connection
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("socket");

    // if system itself fails to create socket, we terminate the process
    exit(EXIT_FAILURE);
  }

  // configuring the socket connection for server

  // AF_INET = IPv4, AF_INET6 = IPv6
  server_addr.sin_family = AF_INET;

  // htons means host to network short (16 bits)
  server_addr.sin_port = htons(PORT);
  // learned from https://beej.us/guide/bgnet/

  // binding to all available network interfaces
  server_addr.sin_addr.s_addr = INADDR_ANY;

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

  // close(server_fd);

  return 0;
}
