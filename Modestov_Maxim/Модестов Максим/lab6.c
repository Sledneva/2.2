#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

char message[] = "set ";	//Запрос, который отправится 
char buf[64];				//Переменная, в которую запишется ответ

int main(int argc, char *argv[])
{
    if ( argc != 3 ) //Если количество аргументов не равно 3
    {
        //Написать подсказку для пользователя
        printf( "usage: %s <key> <value>\n", argv[0] );
        exit(1);
    }
    int sock; //Сокет
    struct sockaddr_in addr; //Адрес сервера

    sock = socket(AF_INET, SOCK_STREAM, 0);//инициализация сокета
    if(sock < 0)//Если ошибка
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET; //Указываем тип адреса
    addr.sin_port = htons(11211); //Указываем порт
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //Указываем сам адрес (INADDR_LOOPBACK = 127.0.0.1)
    printf("Connecting to server...\n");
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(2);
    }
    printf("Connected to server\n");
    strcat(message, argv[1]);
    strcat(message, " 0 60 ");

    char str[100];
    sprintf(str,"%d",strlen(argv[2]));
    strcat(message, str);

    strcat(message, "\r\n");
    strcat(message, argv[2]);
    strcat(message, "\r\n");
    printf(message);
    send(sock, message, strlen(message), 0);//Отправляем  сообщение в сокет
    recv(sock, buf, 12, 0);//Получаем ответ (примерно 12 байт) от сокета и сохраняем в buf
    printf(buf);

    close(sock);//Закрываем сокет

    return 0;
}
