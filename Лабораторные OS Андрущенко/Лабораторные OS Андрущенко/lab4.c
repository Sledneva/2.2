#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <semaphore.h>

#define STAND_NUM 3			//Количество стоек
#define IMIG_IN_HALL_NUM 5	//Количество вмещаемых залом иммигрантов

#define IMIG_NUM 12			//Количество иммигрантов
#define VIEWER_NUM 4		//Количество зрителей

int immigrant_count = 0;	//Количество иммигрантов в зале
int viewer_count = 0;		//Количество зрителей в зале
int sweared_immigrant_count = 0;	//Количество клянущихся иммигрантов в зале
int immigrant_in_stand_count = 0;	//Количество поклявшихся иммигрантов в зале =)
int judge_in_the_hall = 0;	//судья в зале

sem_t sem;

pthread_mutex_t viewer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t immigrant_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t judge_in_the_hall_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t swearing_immigrant_mutex = PTHREAD_MUTEX_INITIALIZER;

void printtime(){
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64], buf[64];
    
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%H:%M:%S", nowtm);
    printf("[%s.%06ld]:", tmbuf, tv.tv_usec);
}

void printhall(){
	if(immigrant_count > 0 || viewer_count > 0)
	{
     printf("В зале: ");
	 if(immigrant_count > 0)
		printf("%d иммигрантов, ",immigrant_count);
	 if(viewer_count > 0)
		printf("%d зрителя.",viewer_count);
	}
	 printf("\n");
}

int limited_random(int max)
{
  return random() % max + 1;
}

void *judge()
{
	while(1)
	{
		//Ждем перед входом
		sleep(limited_random(30));
		
		printtime();
    		printf("Судья зашел в здание.");
		printhall();
	
		sleep(limited_random(5));

		//Ждем пока не будет хоть 1 иммигрант
		while(immigrant_in_stand_count != 0);

		pthread_mutex_lock(&judge_in_the_hall_mutex);
		
		judge_in_the_hall = 1;
		pthread_mutex_lock(&viewer_mutex);
		printtime();
		printf("Судья зашел в зал и начал принимать присягу.");
		printhall();
    	pthread_mutex_unlock(&viewer_mutex);
    	sleep(limited_random(15));

    	//ждем присяги иммигрантов
    	while(sweared_immigrant_count!=immigrant_count);

    	pthread_mutex_lock(&swearing_immigrant_mutex);
    	sweared_immigrant_count  = 0;
    	pthread_mutex_unlock(&swearing_immigrant_mutex);

    	judge_in_the_hall = 0;
		
		printtime();
    	printf("Судья закончил принимать присягу и покинул помещение.\n");
		printhall();
		
    	pthread_mutex_unlock(&judge_in_the_hall_mutex);			
    }
}

void *immigrant(int id)
{
	int immigrant_in_the_hall = 0;

	sleep(limited_random(30));

	printtime();
    	printf("Иммигрант №%d зашел в здание. ", id);
	printhall();

	sleep(limited_random(5));

	do{
		pthread_mutex_lock(&judge_in_the_hall_mutex);
		if(immigrant_count < IMIG_IN_HALL_NUM)
		{
			immigrant_count++;
			immigrant_in_the_hall = 1;
			pthread_mutex_lock(&viewer_mutex);
			printtime();
			printf("Иммигрант №%d зашел в зал. ", id);
			printhall();
    		pthread_mutex_unlock(&viewer_mutex);
    	}
    	pthread_mutex_unlock(&judge_in_the_hall_mutex);
	}while (!immigrant_in_the_hall);
    
    //Ждем судью
    while(!judge_in_the_hall);
    //клятва
    pthread_mutex_lock(&swearing_immigrant_mutex);
    sleep(limited_random(5));
    sweared_immigrant_count++;
    immigrant_in_stand_count++;
    printf("Иммигрант №%d поклялся. Осталось: %d \n", id, immigrant_count - sweared_immigrant_count);
    
    pthread_mutex_unlock(&swearing_immigrant_mutex);

    //выйдет когда судья выйдет
    while(judge_in_the_hall);

    sem_wait(&sem);
	printtime();
	printf("Иммигрант №%d получает справку.", id);
	printhall();
	sleep(limited_random(7));    
    sem_post(&sem);

    pthread_mutex_lock(&judge_in_the_hall_mutex);
    immigrant_count--;
    immigrant_in_stand_count--;
	
	printtime();
    	printf("Иммигрант №8 получил справку и покинул помещение.", id);
	
    pthread_mutex_unlock(&judge_in_the_hall_mutex);
}

void *viewer(int id)
{
	while(1)
	{
		sleep(limited_random(30));

		printtime();
        	printf("Зритель №%d зашел в здание. ", id);
		printhall();

		sleep(limited_random(5));

		pthread_mutex_lock(&judge_in_the_hall_mutex);
		pthread_mutex_lock(&viewer_mutex);
		
		printtime();
        	printf("Зритель №%d зашел в зал. ", id);
		printhall();
		viewer_count++;
		
		pthread_mutex_unlock(&viewer_mutex);
    	pthread_mutex_unlock(&judge_in_the_hall_mutex);		

    	sleep(limited_random(30));

    	pthread_mutex_lock(&viewer_mutex);
		viewer_count--;
        printtime();
        printf("Зритель №%d покинул зал. ", id);
		printhall();
    	pthread_mutex_unlock(&viewer_mutex);		
	}
}


int main(){
    printtime();
    printf("Программа запущена.\n");
    pthread_t judge_thread;
    pthread_t immigrant_thread[IMIG_NUM];
    pthread_t viewer_thread[VIEWER_NUM];
	
	sem_init(&sem, 0, STAND_NUM);
	
    void *judge();
    void *immigrant();
    void *viewer();

    srand(time(NULL));
    
	//Запуск потоков иммигрантов
    for (int i=1; i<=IMIG_NUM; i++){
        pthread_create(&immigrant_thread[i], NULL, immigrant, i);
    }
    //Запуск потоков зритей
    for (int i=1; i<=VIEWER_NUM; i++){
        pthread_create(&viewer_thread[i], NULL, viewer, i);
    }
	//Запуск потока судьи
	pthread_create(&judge_thread, NULL, judge, NULL);
    pthread_join(&judge_thread, NULL);
}



