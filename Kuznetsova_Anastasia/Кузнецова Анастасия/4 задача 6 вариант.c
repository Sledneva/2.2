#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <stddef.h>

void *warden();
void *student(void *studentId);

typedef struct Student{
	bool isInRoom;
} Student;

typedef struct Warden{
	bool isInRoom;
} Warden;

pthread_cond_t warden_condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t warden_mutex = PTHREAD_MUTEX_INITIALIZER;

Student *_students;
Warden _warden;

int room_capacity;
int students_amount;



int main(int argc, char **argv){
	
	students_amount = 6;
	srand(time(NULL));

	if (argc != 2) {
		room_capacity = 4;
	}
	else
	{
		room_capacity = atoi(argv[1]);
	}
	long i;

	pthread_t students_thread[students_amount];
	pthread_t warden_thread;
	
	_students = (Student*)calloc(students_amount, sizeof(Student));

	
	for (i = 0; i < students_amount; i++) {
		pthread_create(students_thread + i, NULL, &student, (void*)i);
	}
	pthread_create(&warden_thread, NULL, warden, NULL);
	
	pthread_join(warden_thread, NULL);
	for (i = 0; i < students_amount; i++) {
		pthread_join(students_thread + i, NULL);
	}
	

	return 0;
}

void *student(void *studentId) {
	long id = studentId;
	for (;;) {
		sleep(1);

		if (!_students[id].isInRoom) {
			int ableToEnter = rand();
			if (ableToEnter % 5 == 0) {
				pthread_mutex_lock(&warden_mutex);
				if (!_warden.isInRoom) {
					_students[id].isInRoom = true;
					int a = getStudentsAmountInRoom();
					print_time_message("Student has entered the room.");
					printf("\t Total: %d \n", a);
				}
				else {
					print_time_message("Student can't enter, warden is in the room");
				}

				pthread_mutex_unlock(&warden_mutex);
			}
		}
		else {
			int ableToLeave = rand();
			if (ableToLeave % 2 == 0) {
				_students[id].isInRoom = false;
				print_time_message("Student has left the room");
				pthread_cond_signal(&warden_condition);
				int a = getStudentsAmountInRoom();
				printf("\t Total: %d \n", a);
			}

		}
	}
}

void *warden() {
	
	for (;;) {
		
		sleep(1);
		// mutex lock
		int studentsAmountInRoom = getStudentsAmountInRoom();
		bool areStudentsLoud = studentsAmountInRoom >= room_capacity;
		if (areStudentsLoud) {
			print_time_message("Warden has entered the room.");
			pthread_mutex_lock(&warden_mutex);
			_warden.isInRoom = true;
			//pthread_mutex_unlock(&commandant_mutex);
			for (;;) {
				pthread_cond_wait(&warden_condition, &warden_mutex);
				int newStudents = getStudentsAmountInRoom();
				if (!newStudents) {
					//pthread_mutex_lock(&commandant_mutex);
					_warden.inRoom = false;
					pthread_mutex_unlock(&warden_mutex);
					print_time_message("Warden has left the room");
					
					break;
				}
			}
		}
		else {
			print_time_message("Warden passed by.");
		}
// mutex unlock
	}
}

int getStudentsAmountInRoom() {
	if (_students == NULL) {
		return 0;
	}
	int i;
	int amount = 0;
	for (i = 0; i < students_amount; i++) {
		if (_students[i].isInRoom) {
			amount++;
		}
	}
	return amount;
}

void print_time_message(char* message) {
	print_time();
	puts(message);
}

void print_time() {
	time_t timer;
	char buffer[26];
	struct tm *tm_info;

	time(&timer);
	tm_info = localtime(&timer);

	strftime(buffer, 26, "%H:%M:%S ", tm_info);
	printf(buffer);
}