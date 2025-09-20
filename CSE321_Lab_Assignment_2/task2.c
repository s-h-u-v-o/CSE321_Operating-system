#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

sem_t w, seats;
pthread_mutex_t m1;
int waiting = 0;
int served = 0;
int st_sleep = 1;
pthread_t t1;
sem_t st;


void *st_func(void *arg);

void *std_func(void *arg);

int main() {
pthread_t td1[10];
int id[10];
int students = 10;

sem_init(&seats, 0, 3);
sem_init(&w, 0, 0);
sem_init(&st, 0, 0);
pthread_mutex_init(&m1, NULL);

for (int i = 0; i < students; i++) {
id[i] = i;
usleep(rand() % 530346);
pthread_create(&td1[i], NULL, std_func, &id[i]);
}

for (int i = 0; i < students; i++) {
pthread_join(td1[i], NULL);
}

sem_destroy(&seats);
sem_destroy(&st);
sem_destroy(&w);
pthread_mutex_destroy(&m1);

return 0;
}


void *st_func(void *arg) {
while (1) {
if (sem_wait(&w) != 0) break;

pthread_mutex_lock(&m1);
printf("A waiting student started getting consultation.\n");
printf("ST giving consultation.\n");
pthread_mutex_unlock(&m1);

sem_post(&st);

pthread_mutex_lock(&m1);
waiting--;
printf("Number of students now waiting: %d\n", waiting);

if (waiting == 0) {
st_sleep = 1;
pthread_mutex_unlock(&m1);
break;
}
pthread_mutex_unlock(&m1);
}
pthread_exit(NULL);
}


void *std_func(void *arg) {
int id = *(int*)arg;

if (sem_trywait(&seats) == 0) {
pthread_mutex_lock(&m1);
waiting++;
printf("Student %d started waiting for consultation.\n", id);
printf("Number of students now waiting: %d\n", waiting);

if (st_sleep == 1) {
st_sleep = 0;
pthread_create(&t1, NULL, st_func, NULL);
}
pthread_mutex_unlock(&m1);

sem_post(&w); 

sem_wait(&st); 

pthread_mutex_lock(&m1);
printf("Student %d is getting consultation.\n", id);
pthread_mutex_unlock(&m1);

sleep(1);

pthread_mutex_lock(&m1);
served++;
printf("Student %d finished getting consultation and left.\n", id);
printf("Number of served students: %d\n", served);
pthread_mutex_unlock(&m1);

sem_post(&seats);
} 
else {
pthread_mutex_lock(&m1);
printf("No chairs remaining in lobby. Student %d Leaving.....\n", id);
pthread_mutex_unlock(&m1);
}

pthread_exit(NULL);
}