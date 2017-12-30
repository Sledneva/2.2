
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define MAXTHREAD 7
#define MAXREAD 5
#define MAXWRITE 3


void access_resource();

void* reader(void*);
void* writer(void*);

sem_t q;
int r_counter = 0;
int w_counter = 0;
int write_request = 0;

int main()
{
    pthread_t readers[MAXREAD],writes[MAXWRITE];
    int index;
    int readersIds[MAXREAD];
    sem_init (&q,0,1);
    for(index = 0; index < MAXREAD; index ++) {
        readersIds[index]=index+1;
        if(pthread_create(&readers[index],0,reader,&readersIds[index])!=0){
            perror("Невозможно создать читателя!");
            _exit(1);
        }
    }

    int writerindex;
    int writersIds[MAXWRITE];
    for (writerindex = 0; writerindex < MAXWRITE; writerindex++) {
        writersIds[writerindex] = writerindex + 1;
        if (pthread_create(&writes[index], 0, writer, &writersIds[writerindex]) != 0) {
            perror("Невозможно создать писателя!");
            _exit(1);
        }
    }

    pthread_join(writes,0);
    sem_destroy (&q);
    return 0;
}

void* reader(void *arg)
{
    srand(time(NULL));
    int index = *(int*)arg;
    int can_read;
    while(1) {
        can_read = 1;

        access_request();

        sem_wait(&q);
        if(w_counter == 0 && write_request == 0 && r_counter < MAXREAD) {
            r_counter++;
            printf("Читатель %d хочет читать. | Ресурс свободен.\n", index);
        }
        else {
            can_read = 0;
            printf("Читатель %d хочет читать. | Ресурс занят.\n", index);
        }
        sem_post(&q);

        if(can_read) {
            printf("Читатель %d читает.\n", index);
            access_resource();

            sem_wait(&q);
            r_counter--;
            sem_post(&q);
            printf("Читатель %d прекратил читать.\n", index);
        }

        sched_yield();
    }
    return 0;
}
;
void* writer(void *arg)
{
    srand(time(NULL));
    int index = *(int*)arg;
    int can_write;
    while(1){
        can_write = 1;

        access_request();

        sem_wait (&q);
        if(r_counter == 0 && w_counter == 0) {
            w_counter++;
            printf("Писатель %d хочет писать. | Ресурс свободен.\n", index);
        }
        else {
            can_write = 0;
            printf("Писатель %d хочет писать. | Количество читателей %d\n", index, r_counter);
        }
        write_request++;
        sem_post(&q);

        if(can_write) {
            printf("Писатель %d пишет\n",index);
            access_resource();

            sem_wait(&q);
            w_counter--;
            write_request--;
            sem_post(&q);
            printf("Писатель %d прекратил писать\n", index);
        }

        sched_yield();
    }
    return 0;
}

void access_resource() {
    sleep(rand() % 16);
}
