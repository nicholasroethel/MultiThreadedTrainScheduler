#include <pthread.h>
#include <stdio.h>
#include <zconf.h>

#define threadNum 3

int counter=0;
pthread_mutex_t mt;

void* count(void* v){
    //pthread_mutex_lock(&mt);//
    counter++;
    printf("I am thread %d, I'm sleeping...\n", counter);
    sleep(1);
    printf("I am thread %d, I finish sleep\n", counter);
    //pthread_mutex_unlock(&mt);//
}

int main(){
    pthread_t pthreads[3];
    pthread_mutex_init(&mt,NULL);
    int i=0;
    for(i=0;i<threadNum;i++){
        pthread_create(&pthreads[i],NULL,count,NULL);
    }
    for(i=0;i<threadNum;i++){
        pthread_join(pthreads[i],NULL);
    }
    pthread_mutex_destroy(&mt);
    return 0;
}
