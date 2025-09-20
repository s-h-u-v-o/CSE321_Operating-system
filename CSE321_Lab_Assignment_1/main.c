#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
pid_t a = fork();
if(a == 0){
execv("./sort", argv);
exit(EXIT_SUCCESS);
}

wait(NULL);
execv("./oddeven", argv);
exit(EXIT_SUCCESS);
}
