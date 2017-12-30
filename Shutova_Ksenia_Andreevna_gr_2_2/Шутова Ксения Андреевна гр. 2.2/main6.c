#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#define PORT_NO 6379
#define ADDRESS "127.0.0.1"
#define GET "get"
#define SET "set"

char key[256];
char value[256];
char command[256];
char tmp[256] = "test";
int socket_fd;

void get_redis_string(char* string, char* redis_string) {
  redis_string += sprintf(redis_string, "$%d\r\n", strlen(string));
  redis_string += sprintf(redis_string, "%s\r\n", string); 
} 

void get_redis_array_header(int length, char* redis_string) {
  sprintf(redis_string, "*%d\r\n", length);
}

int main() {
  struct sockaddr_in server_addr;
  printf("Trying to connect to Redis server\n");
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    printf("Unable to open the socket\n");
    return -1;
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NO);
  if (inet_pton(AF_INET, ADDRESS, &server_addr.sin_addr) != 1) {
    printf("Error while address converting\n");
    return -1;
  }

  if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
    printf("Failed to connect to server.\n");
    return -1;
  }

  while(1) {
    printf("Enter a command (set, get, exit):\n");
    scanf(" %s", command);
    if (strcmp(command, "set") == 0) {
      printf("Enter a key to be set:\n");
      scanf(" %s", key);
      printf("Enter a value for key \"%s\":\n", key);
      scanf(" %s", value);
      get_redis_array_header(3, &tmp);
      write(socket_fd, tmp, strlen(tmp));
      get_redis_string(SET, &tmp);
      write(socket_fd, tmp, strlen(tmp));
      get_redis_string(key, &tmp);
      write(socket_fd, tmp, strlen(tmp));
      get_redis_string(value, &tmp);
      write(socket_fd, tmp, strlen(tmp));
      printf("Value \"%s\" succesfully written\n", value);
      read (socket_fd, tmp, 3331);
    }
    else if (strcmp(command, "get") == 0) {
      printf("Enter a key to read:\n");
      scanf(" %s", key);
      get_redis_array_header(2, &tmp);
      write(socket_fd, tmp, strlen(tmp));
      get_redis_string(GET, &tmp);
      write(socket_fd, tmp, strlen(tmp));
      get_redis_string(key, &tmp);
      write(socket_fd, tmp, strlen(tmp));
      read (socket_fd, tmp, 3331);
      printf("%s\n", tmp);
    }
    else if (strcmp(command, "exit") == 0) {
      printf("Exiting...\n");
      break;
    }
    else {
      printf("Unknown command\n");
    }
  }
  return 0;
}
