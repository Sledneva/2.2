
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
/*Вариант 2:
Соединиться с сервером [Memcached](http://memcached.org/) и записать, используя тестовый протокол,
 значение "тест" по ключу "ключ". Проверить наличие значения с помощью программы `telnet`.*/

char setKey[] = "set ";
char setValue[] = "";
char length[10];
char buf[64];


int main(int argc, char *argv[])
{
    if ( argc != 4 )
    {
        printf( "Usage: %s <key> <expiration time> <value>\n", argv[0] );
        exit(1);
    }
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(11211);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //INADDR_LOOPBACK = 127.0.0.1

    printf("Connecting to Memcached...\n");
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Can't connect");
        exit(2);
    }

    printf("Connected to Memcached\n");
    strcat(setKey, argv[1]); //set key
    strcat(setKey, " 0 "); //set key 0
    strcat(setKey, argv[2]); //set key 0 *time*
    strcat(setKey, " ");
    sprintf(length, "%d", strlen(setValue));
    strcat(setKey, length); //set key 0 *time* *length*
    strcat(setKey, "\r\n");
    strcat(setValue, argv[3]);
    strcat(setValue, "\r\n");
    printf(setKey);
    send(sock, setKey, strlen(setKey), 0);
    send(sock, setValue, strlen(setValue), 0);
    
    recv(sock, buf, 12, 0);
    printf(buf);

    close(sock);//Закрываем сокет

    return 0;
}
