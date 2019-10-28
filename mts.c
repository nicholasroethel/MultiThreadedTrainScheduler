#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 

struct Train {  //struct for train queue
    char direction; 
    int loadTime;
    int crossTime;
};

struct List {  //struct for train queue
    struct Train* train;
    struct Node* next; 
} loading, eastTrain, westTrain, track;

void *PrintHello(void *threadid)
{
 long tid;
 tid = (long)threadid;
   printf("Hello World! It's me, thread #%ld!\n", tid);
 pthread_exit(NULL);
}

void wait(int seconds) //waits for the amount of seconds passed through
{ 
    int milliSeconds = 1000 * seconds; 
    clock_t startTime = clock(); 
    while (clock() < startTime + milliSeconds); 
} 

int main(int argc, char *argv[]){
  //setup for tokenization
  const char delim[2] = " ";
  char *token;

  //loading in the file
  char const* const fileName = argv[1]; 
  FILE* trainFile = fopen(fileName, "r"); 
  char line[256];

  int trainCount = 0; //count to see how many trains are in the file

  //iterate through the file and create the trains
  while (fgets(line, sizeof(line), trainFile) != NULL) {

    //allocate memory for the train
    struct Train *temp = ( struct Train * )malloc( sizeof( struct Train ) );

    //get the trains direction
    token = strtok(line, delim);
    temp->direction = *token;
    printf( "%s ", token );

    //get the trains load time
    token = strtok(NULL, delim);
    temp->loadTime = *token;
    printf( "%s ", token );

    //get the trains crossing time
    token = strtok(NULL, delim);
    temp->crossTime = *token;
    printf( "%s \n", token ); 

    //increment train counter
    trainCount++; 
    
  }
  //close the file
  fclose(trainFile);

  //the amount of threads for the train
  int NUM_THREADS = trainCount;


  pthread_t threads[NUM_THREADS];
  int rc;
  long t;

  wait(5);
    
  for(t=0;t<NUM_THREADS;t++){
    printf("In main: creating thread %ld\n", t);
      rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
      if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
      }
  }

/* Last thing that main() should do */
 pthread_exit(NULL);
}
