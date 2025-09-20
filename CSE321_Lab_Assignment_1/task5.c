#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
printf("Parent process ID: 0\n");
pid_t child = fork();
if (child == 0) {
printf("Child process ID: %d\n", getpid());

for (int i = 0; i < 3; i++) {
pid_t gc = fork();
if (gc == 0) {
printf("Grand Child process ID: %d\n", getpid());
return 0;
}
}

for (int i = 0; i < 3; i++) {
wait(NULL);
}

}

return 0;
}
