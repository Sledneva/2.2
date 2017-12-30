/*
Вариант 1:
Необходимо решить проблему "Санта Клаус" с использованием библиотеки PTHREAD с учетом следующих ограничений:
●	Санта все время спит, пока его не будет либо все его олени (число оленей — N), либо K из его M эльфов.
●	Если его будят олени, он впрягает их в сани, развозит игрушки в течение какого-то случайного времени, после чего распрягает их и отправляет погулять.
●	Каждый олень гуляет случайное время.
●	Если его будят эльфы, он проводит их по одному в свой кабинет, обсуждает с ними новые игрушки, а потом по одному отпускает.
●	Время обсуждения равно T. Каждый эльф после обсуждения с Сантой занимается своими делами случайное время.
●	Санта должен сначала поехать с оленями, если и олени и эльфы ждут его у дверей.
Результаты работы программы должны отображаться по мере их появления следующим образом. Вывод должен быть синхронизированным, т.е. последовательность строк в логе должна соответствовать последовательности операций, выполняемых программой.
...
12:01:10.353 Санта спит.
12:01:11.412 Эльф 5 подошел к двери. Ожидающих эльфов: 1. Ожидающих оленей: 0.
12:01:11.700 Эльф 1 подошел к двери. Ожидающих эльфов: 2. Ожидающих оленей: 0.
12:01:11.701 Олень 7 подошел к двери. Ожидающих эльфов: 2. Ожидающих оленей: 1.
12:01:11.910 Олень 8 подошел к двери. Ожидающих эльфов: 2. Ожидающих оленей: 2.
12:01:11.815 Олень 9 подошел к двери. Ожидающих эльфов: 2. Ожидающих оленей: 3.
12:01:14.810 Эльф 3 подошел к двери. 3 эльфа будят Санту.
12:01:14.951 Санта пропускает в кабинет эльфа 1.
12:01:15.115 Санта пропускает в кабинет эльфа 5.
12:01:15.222 Санта пропускает в кабинет эльфа 3.
12:01:17.815 Олень 3 подошел к двери. Ожидающих эльфов: 0. Ожидающих оленей: 4.
12:01:17.900 Санта с эльфами начинают совещание.
...
Параметры N, M, K и T задаются пользователем.
*/

#define _CRT_SECURE_NO_WARNINGS
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t door_mutex;

pthread_cond_t knock_to_santa;
pthread_cond_t santa_admite_elf, santa_kick_elf;
pthread_cond_t end_of_deer_fly;

int deer_count, elf_count, N, M, K, T; 

int randomtime()
{
    return 10*1000 + rand() % (10*1000*1000);
}

void printtime(){
    
    //struct timeval tv;
    //time_t nowtime;
    //struct tm *nowtm;
    //char tmbuf[64], buf[64];
    
    //gettimeofday(&tv, NULL);
    //nowtime = tv.tv_sec;
    //nowtm = localtime(&nowtime);
    //strftime(tmbuf, sizeof tmbuf, "%H:%M:%S", nowtm);
    //printf("[%s.%06ld]:", tmbuf, tv.tv_usec);

    //long int s_time;
    //struct tm *m_time;
    //s_time = time(NULL);
    //m_time = localtime(&s_time);
    //printf("%s\n", asctime(m_time));

    //printf("%02d:%02d:%02d - ");

    printf("Now - ");
}


int main()
{
    printf("Введите задаваемые числа задачи (N M K T): ");
    scanf("%d %d %d %d", &N, &M, &K, &T);
    if(K>M)
        K=M;

    printf("Программа запустится с параметрами: %d оленей, %d эльфов всего, %d эльфов для совещания, %d единиц времени проводится совещание.\n\n", N, M, K, T*1000*1000);

    deer_count = 0;
    elf_count = 0;

    pthread_mutex_init(&door_mutex, NULL);

    pthread_cond_init(&knock_to_santa, NULL);
    pthread_cond_init(&end_of_deer_fly, NULL);
    pthread_cond_init(&santa_admite_elf, NULL);
    pthread_cond_init(&santa_kick_elf, NULL);

    pthread_t santa_thread, elf_threads[M], deer_threads[N];    
    void *elf(), *deer(), *santa();
    pthread_create(&santa_thread, NULL, santa, NULL);

    for(int i = 0; i<M; i++)
    {
        pthread_create(&elf_threads[i], NULL, elf, i);
    }

    for(int i = 0; i<N; i++)
    {
        pthread_create(&deer_threads[i], NULL, deer, i);
    }

    pthread_join(santa_thread, NULL);
}

void try_to_knock()
{
    if(deer_count==N)
    {
        pthread_cond_signal(&knock_to_santa);
        //printtime();
        //printf("Олени пробудили древнее зло!\n");
    }

    if(elf_count>=K)
    {
        pthread_cond_signal(&knock_to_santa);
        //printtime();
        //printf("Эльфы пробудили древнее зло!\n");
    }
}

void *elf(int id)
{
    while(1)
    {
        int deal = randomtime();

        //printtime();
        //printf("Эльф-%d сделает %d ед.игрушек и уже заранее заебался\n", id, deal);

        usleep(deal);
        
        pthread_mutex_lock(&door_mutex);
        elf_count++;
        printtime();
        printf("Эльф-%d подошёл к двери: ожидающих эльфов: %d, ожидающих оленей: %d\n", id, elf_count, deer_count);
        pthread_mutex_unlock(&door_mutex);

        try_to_knock();

        pthread_cond_wait(&santa_admite_elf, &door_mutex);

        printtime();
        printf("Эльф-%d зашёл к Санте\n", id);
        pthread_cond_wait(&santa_kick_elf, &door_mutex);

        printtime();
        printf("Эльф-%d: Что это было сейчас?\n", id);
        pthread_mutex_unlock(&door_mutex); // а точно надо? нннада - без не работает
    }
}

void *deer(int id)
{
    while(1)
    {
        int food = randomtime();
        //printtime();
        //printf("Олень-%d отправляется пастись\n", id);
        usleep(food);
        //printtime();
        //printf("Олень-%d пасся %d ед.вр и устал \n", id, food);

        pthread_mutex_lock(&door_mutex);
        deer_count++;
        printtime();
        printf("Олень-%d подошёл к двери: ожидающих эльфов: %d, ожидающих оленей: %d\n", id, elf_count, deer_count);
        pthread_mutex_unlock(&door_mutex);

        try_to_knock();

        //ждать конца поездки
        pthread_cond_wait(&end_of_deer_fly, &door_mutex);
        //printtime();//printf("Олень-%d свободен\n", id);
        pthread_mutex_unlock(&door_mutex); // а точно надо? нннада - без не работает
    }
}

void *santa()
{
    printtime();
    printf("Санта родился!\n");

    while(1)
    {
        printtime();
        printf("Санта уснул\n");
        pthread_cond_wait(&knock_to_santa, &door_mutex);

        while(deer_count==N||elf_count>=K)
        {   
            if(deer_count==N)
            {
                deer_count = 0;
                int fly = randomtime();
                printtime();
                printf("Санта улетел с оленями на %d ед.вр\n", fly);
                pthread_mutex_unlock(&door_mutex);
                usleep(fly);
                pthread_cond_broadcast(&end_of_deer_fly);
                pthread_mutex_lock(&door_mutex);
            }

            if(elf_count>=K)
            {
                int into_elf_count = 0;
                printtime();
                printf("Санта запускает эльфов\n");
                
                while(elf_count>0)
                {
                    printtime();
                    printf("Санта запускает эльфa\n");

                    pthread_cond_signal(&santa_admite_elf);
                    elf_count--;
                    into_elf_count++;
                    pthread_mutex_unlock(&door_mutex);
                }

                printtime();
                printf("Санта и эльфы начинают совещание\n");

                usleep(T*1000*1000);

                printtime();
                printf("Санта и эльфы заканчивают совещание\n");

                while(into_elf_count>0)
                {
                    pthread_mutex_lock(&door_mutex);
                    pthread_cond_signal(&santa_kick_elf);
                    into_elf_count--;
                    printtime();
                    printf("Эльф вышел. Осталось: %d\n", into_elf_count);
                    pthread_mutex_unlock(&door_mutex);
                }

                printtime();
                printf("Санта: Теперь можно и поспать\n");

            }
        }

        pthread_mutex_unlock(&door_mutex);
    }
}
