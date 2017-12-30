

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main()
{
    char* message[4] = {"USER natasha localhost localhost natasha\n","NICK natasha\n","JOIN #test\n", "PRIVMSG #test test\n"}; //Запрос на сервер
    char buf[256];
    int sock; //Сокет
    struct sockaddr_in addr; //Адрес сервера
    sock = socket(AF_INET, SOCK_STREAM, 0);//инициализируем сокет(задаем стандартные значения: AF_INET - семейство адресов для сетевого сокета, SOCK_STREAM-тип(потоковый сокет), PROTOCOL = 0 стандартное значение )
    if(sock < 0)//Если сокет инициализировался с ошибкой
    {
        perror("socket");
        exit(1);
    }
    addr.sin_family = AF_INET; //Указываем тип адреса
    addr.sin_port = htons(6667); //Указываем порт
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //Указываем сам адрес
    printf("Creating a connection to the server...\n");
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)//Если соединение сокета с адресом не удалось
    {
        perror("connect");
        exit(2);
    }
    printf("Connection to the server is established\n"); 
    for(int i=0; i<4; i++){
    if (send(sock, message[i], strlen(message[i]), 0) == -1){
		perror("send error");
		return -1;
		}
    sleep(10);
    }
    recv(sock, buf, 256, 0);
    fprintf(stderr, "%s",buf);//для того чтобы напечатать buf
    close(sock);//Закрываем сокет
    return 0;
}
