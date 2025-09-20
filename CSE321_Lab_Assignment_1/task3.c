#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>

int main() {
int *count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
MAP_SHARED | MAP_ANONYMOUS, -1, 0);
*count = 0;
pid_t a,b,c;
a = fork();
b = fork();
c = fork();
if (a == 0 && getpid() % 2 != 0){
fork();}
if (b == 0 && getpid() % 2 != 0){
fork();}
if (c == 0 && getpid() % 2 != 0){
fork();}
(*count)++;
if (a != 0 && b != 0 && c != 0){
while (wait(NULL) > 0);
printf("%d", *count);
}
exit(EXIT_SUCCESS);
}
