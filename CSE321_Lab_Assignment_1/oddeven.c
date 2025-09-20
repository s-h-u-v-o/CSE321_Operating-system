#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(int argc, char *argv[]){

for (int a = 1; a < argc; a = a + 1)
{
int num = atoi(argv[a]);
if(num % 2 == 0)
printf("%d is Even\n", num);
else
printf("%d is Odd\n", num);
}

}
