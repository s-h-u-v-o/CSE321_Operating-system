#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char *argv[]){
char var[64] ;
strcpy(var,argv[1]);
strcat(var, ".txt");
int asdf = open(var, O_CREAT | O_WRONLY, 0642);
char buffer[1024];
while (true){
ssize_t bytes_read = read(0, buffer, 64);
if (bytes_read <= 0) break;

buffer[bytes_read] = '\0';

if (strcmp(buffer, "-1\n") == 0 || strcmp(buffer, "-1") == 0) {
continue;
}
write(asdf, buffer, bytes_read);
break;
}

close(asdf);
}
