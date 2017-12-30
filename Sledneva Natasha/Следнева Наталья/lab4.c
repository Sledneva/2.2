#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int TIME;
char buf[256];
time_t time_now;


enum state_f { freed, busy };
struct fork{
	enum state_f state;
	pthread_mutex_t mutex;
};

enum state_p { suppose, eat, die };
struct philosopher{
	pthread_mutex_t mutex;
	pthread_t thr;
	pthread_t life_thr;
	int last_eat_t;
	enum state_p state;    
	struct fork* fork_l;
	struct fork* fork_r;
	int left_fork_ph;
	int right_fork_ph;
};

struct philosopher philosophers[5];
struct fork forks[5];

void *philosopher_t(void* num);
void *last_eat_t(void* num);

void state_f_pr(){
	printf("Состояние вилок: ");
	for(int i = 0; i<5; i++){
		printf("%d", forks[i].state);
	}
	printf("\n");
}

void start_eat(int n){
	pthread_mutex_lock(&philosophers[n].mutex);
	philosophers[n].state = eat;
	time_now = time(NULL);
	strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
	printf("%s Философ %d начал есть. \n", buf, n+1);
}

void end_eat(int n){
	philosophers[n].state = suppose;
	philosophers[n].last_eat_t = 0;
	time_now = time(NULL);
	strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
	printf("%s Философ %d закончил есть. \n", buf, n+1);
	pthread_mutex_unlock(&philosophers[n].mutex);
}
void *philosopher_t(void* num){
	int n = (int)(intptr_t)num;
	while (philosophers[n].state != die){
		int rnd = TIME % 2;
		switch(rnd){
		case 0:
			pthread_mutex_lock(&philosophers[n].fork_l->mutex);
			if (philosophers[n].left_fork_ph){
				philosophers[n].fork_l->state = freed;
				philosophers[n].left_fork_ph = 0;
				time_now = time(NULL);
				strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
				printf("%s Философ %d положил левую вилку. ", buf, n+1);
				state_f_pr();
				if (philosophers[n].state == eat){
					end_eat(n);
				}
			} 
			else{
				if (philosophers[n].fork_l->state == freed){
					philosophers[n].fork_l->state = busy;
				 	philosophers[n].left_fork_ph = 1;
					time_now = time(NULL);
					strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
					printf("%s Философ %d взял левую вилку. ", buf, n+1);
					state_f_pr();
					if (philosophers[n].right_fork_ph){
						start_eat(n);
					}	
				}
				else{
					time_now = time(NULL);
					strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
					printf("%s Философ %d не смог взять левую вилку. ", buf, n+1);
					state_f_pr();
				}
			}
			pthread_mutex_unlock(&philosophers[n].fork_l->mutex);		
			break;
		case 1:
			pthread_mutex_lock(&philosophers[n].fork_r->mutex);
			if (philosophers[n].right_fork_ph){
				philosophers[n].fork_r->state = freed;
				philosophers[n].right_fork_ph = 0;
				time_now = time(NULL);
				strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
				printf("%s Философ %d положил правую вилку. ", buf, n+1);
				state_f_pr();
				if (philosophers[n].state == eat){
					end_eat(n);
				}
			} 
			else{
				if (philosophers[n].fork_r->state == freed){
					philosophers[n].fork_r->state = busy;
					philosophers[n].right_fork_ph = 1;
					time_now = time(NULL);
					strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
					printf("%s Философ %d взял правую вилку. ", buf, n+1);
					state_f_pr();
					if (philosophers[n].left_fork_ph){
						start_eat(n);
					}
				}
				else{
					time_now = time(NULL);
					strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
					printf("%s Философ %d не смог взять правую вилку. ", buf, n+1);
					state_f_pr();
				}
			}
			pthread_mutex_unlock(&philosophers[n].fork_r->mutex);		
			break;
		}
		int sleeep = rand() % 5;
		sleep(sleeep);
	}
	pthread_exit(NULL);
}



void *last_eat_t(void* num){
	int n = (int)(intptr_t)num;
	while (philosophers[n].state != die){
		while (philosophers[n].last_eat_t <= TIME){
			sleep(1);
			philosophers[n].last_eat_t++;
		};
		if (!pthread_mutex_trylock(&philosophers[n].mutex)){
			philosophers[n].state = die;
			philosophers[n].fork_l->state = freed;
			philosophers[n].fork_r->state = freed;
			time_now = time(NULL);
			strftime(buf, 20, "%H:%M:%S", localtime(&time_now));
			printf("%s Философ %d умер\n", buf, n+1);
			pthread_mutex_unlock(&philosophers[n].mutex);
		}
	}
	pthread_exit(NULL);
}
int main(){
	srand(time(NULL));
	printf("%s\n", "Время жизни без еды:");
	scanf("%d", &TIME);
	for (int i=0; i<5; i++){
		philosophers[i].state = suppose; 
		philosophers[i].last_eat_t = 0;
		philosophers[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		forks[i].state = freed;
		forks[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		if (i==0)
			philosophers[i].fork_l = &forks[4];
		else 
			philosophers[i].fork_l = &forks[i-1];
		if (i==4)
			philosophers[i].fork_r = &forks[0];
		else 
			philosophers[i].fork_r = &forks[i];	
		philosophers[i].left_fork_ph = 0;
		philosophers[i].right_fork_ph = 0;
		pthread_create(&(philosophers[i].thr), NULL, philosopher_t, (void*)(intptr_t)i);
		pthread_create(&(philosophers[i].life_thr), NULL, last_eat_t, (void*)(intptr_t)i);
	}
	for (int i=0; i<5; i++){
		pthread_join(philosophers[i].thr, NULL);
	}
	return 0;
}
