#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

struct shared {
    char sel[100];
    int b;
};

int main() {
    key_t key = ftok("shmfile",65); // key for shared memory
    int shmid = shmget(key, sizeof(struct shared), 0666|IPC_CREAT);
    struct shared *sh = (struct shared*) shmat(shmid, NULL, 0);

    int fd[2];
    pipe(fd);

    printf("Provide Your Input From Given Options:\n");
    printf("1. Type a to Add Money\n");
    printf("2. Type w to Withdraw Money\n");
    printf("3. Type c to Check Balance\n");

    char input[100];
    scanf("%s", input);

    strcpy(sh->sel, input);
    sh->b = 1000;

    printf("Your selection: %s\n\n", sh->sel);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    else if (pid == 0) {
        close(fd[0]);

        if (strcmp(sh->sel, "a") == 0) {
            int amt;
            printf("Enter amount to be added:\n");
            scanf("%d", &amt);
            if (amt > 0) {
                sh->b += amt;
                printf("Balance added successfully\n");
                printf("Updated balance after addition:\n%d\n", sh->b);
            } else {
                printf("Adding failed, Invalid amount\n");
            }
        }
        else if (strcmp(sh->sel, "w") == 0) {
            int amt;
            printf("Enter amount to be withdrawn:\n");
            scanf("%d", &amt);
            if (amt > 0 && amt < sh->b) {
                sh->b -= amt;
                printf("Balance withdrawn successfully\n");
                printf("Updated balance after withdrawal:\n%d\n", sh->b);
            } else {
                printf("Withdrawal failed, Invalid amount\n");
            }
        }
        else if (strcmp(sh->sel, "c") == 0) {
            printf("Your current balance is:\n%d\n", sh->b);
        }
        else {
            printf("Invalid selection\n");
        }

        char msg[] = "Thank you for using\n";
        write(fd[1], msg, strlen(msg)+1);
        close(fd[1]);

        shmdt(sh);
        exit(0);
    }
    else {
        close(fd[1]);
        wait(NULL);

        char buffer[200];
        read(fd[0], buffer, sizeof(buffer));
        printf("%s", buffer);

        close(fd[0]);

        shmdt(sh);
        shmctl(shmid, IPC_RMID, NULL);
    }

    return 0;
}