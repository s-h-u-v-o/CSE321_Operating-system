#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void *sequence (void *arg);
void *search (void *arg);

int n;
int s;
int *seq;

int main(){
    pthread_t t1;
    pthread_t t2;

    printf("Enter the term of fibonacci sequence: \n");
    scanf("%d",&n);
    while (n < 0 || n > 40){
        printf("Enter a number between 0 and 40: \n");
        scanf("%d",&n);
    }

    pthread_create(&t1, NULL, sequence, (void*)&n);
    pthread_join(t1, (void*)&seq);

    printf("How many numbers you are willing to search?: \n");
    scanf("%d",&s);
    while(s <= 0){
        printf("Enter a number greater than 0: ");
        scanf("%d",&s);
    }

    int arr[n+3];
    arr[0] = n;
    arr[1] = s;
    int i = 1;
    while (i <= n+1){
        arr[i+1] = seq[i-1];
        i++;
    }

    pthread_create(&t2, NULL, search, (void*)&arr);
    pthread_join(t2, NULL);
    return 0;
}


void *sequence (void *arg){
    int n = *(int*)arg;
    int *seq = malloc((n+1)* sizeof(int));
    seq[0] = 0;
    seq[1] = 1;

    int j = 2;
    while (j <= n){
        seq[j] = seq[j-1] + seq[j-2];
        j++;
    }

    int k = 1;
    while (k <= n+1){
        printf("a[%d]: %d\n", k-1, seq[k-1]);
        k++;
    }
    pthread_exit(seq);
}


void *search (void *arg){
    int *seq = (int*)arg; 
    int n = seq[0];
    int s = seq[1];
    for (int i=0; i<s; i++){
        int x;
        printf("Enter search %d: ", i+1);
        scanf("%d",&x);
        if (x > n){
            printf("result of search #%d = -1\n", i+1);}
        else{
            printf("result of search #%d = %d\n", i+1, (seq[x+2]));
        }
    }
    pthread_exit(NULL);
}