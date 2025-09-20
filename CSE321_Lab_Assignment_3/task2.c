#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

struct msg {
    long int type;
    char txt[6];
};

int main() {
    key_t key = ftok("msgfile", 65);

    int msgid = msgget(key, 0666);
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
    }

    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget failed");
        exit(1);
    }

    struct msg ms;

    printf("Please enter the workspace name:\n");
    char ws[20];
    scanf("%s", ws);

    if (strcmp(ws, "cse321") != 0) {
        printf("Invalid workspace name\n");
        msgctl(msgid, IPC_RMID, NULL);
        exit(0);
    }

    ms.type = 1;
    strncpy(ms.txt, ws, sizeof(ms.txt)-1);
    ms.txt[sizeof(ms.txt)-1] = '\0';
    msgsnd(msgid, &ms, sizeof(ms.txt), 0);

    printf("Workspace name sent to otp generator from log in: %s\n\n", ws);

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid1 == 0) {
        msgrcv(msgid, &ms, sizeof(ms.txt), 1, 0);
        printf("OTP generator received workspace name from log in: %s\n\n", ms.txt);

        pid_t otp = getpid();
        snprintf(ms.txt, sizeof(ms.txt), "%d", otp);

        ms.type = 2;
        msgsnd(msgid, &ms, sizeof(ms.txt), 0);
        printf("OTP sent to log in from OTP generator: %s\n", ms.txt);

        ms.type = 3;
        msgsnd(msgid, &ms, sizeof(ms.txt), 0);
        printf("OTP sent to mail from OTP generator: %s\n", ms.txt);

        pid_t pid2 = fork();
        if (pid2 < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid2 == 0) {
            msgrcv(msgid, &ms, sizeof(ms.txt), 3, 0);
            printf("Mail received OTP from OTP generator: %s\n", ms.txt);

            ms.type = 4;
            msgsnd(msgid, &ms, sizeof(ms.txt), 0);
            printf("OTP sent to log in from mail: %s\n", ms.txt);

            exit(0);
        } else {
            wait(NULL);
            exit(0);
        }
    }
    else {
        wait(NULL);

        char otp1[6], otp2[6];

        msgrcv(msgid, &ms, sizeof(ms.txt), 2, 0);
        strcpy(otp1, ms.txt);
        printf("Log in received OTP from OTP generator: %s\n", otp1);

        msgrcv(msgid, &ms, sizeof(ms.txt), 4, 0);
        strcpy(otp2, ms.txt);
        printf("Log in received OTP from mail: %s\n", otp2);

        if (strcmp(otp1, otp2) == 0) {
            printf("OTP Verified\n");
        } else {
            printf("OTP Incorrect\n");
        }

        msgctl(msgid, IPC_RMID, NULL);
    }

    return 0;
}
