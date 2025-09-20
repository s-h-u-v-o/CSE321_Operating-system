#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    pid_t pid, pid1;
    int status;

    pid = fork(); 

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        pid1 = fork();
        if (pid1 == 0){
          printf("I am grandchild\n");
          exit(EXIT_SUCCESS); }
        wait(NULL);
        printf("I am child\n");
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL);
        printf("I am parent\n");
    }

    return EXIT_SUCCESS;
}

