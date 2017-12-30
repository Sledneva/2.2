#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>


int main() {

    char *file_path = "/var/run/collectd-unixsock";

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("Can not create socket!");
        exit(1);
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, file_path, sizeof(addr.sun_path));
    int size = sizeof(addr);

    if (connect(sock, (struct sockaddr *)&addr, size) < 0) {
        perror("Can not connect to socket!");
        exit(2);
    }

    char *message = "PUTVAL localhost.localdomain/interface-lo/if_octets 1179574444:123:456\n";
    char answer[1024];

    send(sock, message, strlen(message), 0);
    int received = recv(sock, answer, sizeof(answer), 0);
    answer[received] = '\0';
    printf(answer);

    close(sock);

    return 0;
}
