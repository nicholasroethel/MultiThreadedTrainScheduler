#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t waitingLock =  PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t waitingCond = PTHREAD_COND_INITIALIZER;
bool ready = true;
bool track = false;

typedef struct Train {  //struct for train queue
    long int id;
    char direction; 
    long int loadTime;
    long int crossTime;
}Train;

typedef struct loading {  //struct for the loading train queue
    struct Train train;
    struct loading* next; 
}loading;

typedef struct waiting{  //struct for the trains waiting to go on the track queue
    struct Train train;
    struct waiting* next; 
}waiting;


void *PrintHello(void *threadid)
{
 long tid;
 tid = (long)threadid;
   printf("Hello World! It's me, thread #%ld!\n", tid);
 pthread_exit(NULL);
}


struct loading* addToLoadingQueue(struct loading *loadingHead, struct loading *loadingCurrent,struct loading *loadingCurrent, struct Train tempTrain){

  struct loading* loadingNew = ( struct loading * )malloc( sizeof( struct loading ) );
  loadingNew->train.id = tempTrain.id;
  loadingNew->train.direction = tempTrain.direction;
  loadingNew->train.loadTime = tempTrain.loadTime;
  loadingNew->train.crossTime = tempTrain.crossTime;
  loadingNew->next = NULL;

  if(loadingHead == NULL){
    loadingHead = loadingNew;
  }
  else{
    loadingCurrent = loadingHead;
    while(loadingCurrent->next!=NULL){
      loadingCurrent = loadingCurrent->next; 
    }
    loadingCurrent->next = loadingNew;
  }

  return loadingHead;
}


 struct loading* readFile(struct loading *loadingHead, char const* const fileName){

  FILE* trainFile = fopen(fileName, "r"); 
  char line[256];

  //setup for tokenization
  const char delim[2] = " ";
  char *token;

  printf("Trains Read:\n");

  long trainCount = 1;
  //iterate through the file and create the trains
  while (fgets(line, sizeof(line), trainFile) != NULL) {

    //allocate memory for the train
    struct Train *tempTrain = ( struct Train * )malloc( sizeof( struct Train ) );

    tempTrain->id = trainCount;
    printf("%ld ",trainCount);

    //get the trains direction
    token = strtok(line, delim);
    tempTrain->direction = *token;
    printf( "%s ", token );

    //get the trains load time
    token = strtok(NULL, delim);
    tempTrain->loadTime = atoi(token);
    printf( "%s ", token );

    //get the trains crossing time
    token = strtok(NULL, delim);
    tempTrain->crossTime = atoi(token);
    printf( "%s", token ); 
 
    loadingHead = addToLoadingQueue(loadingHead,loadingCurrent,*tempTrain);
    trainCount++;
  }
  //close the file
  fclose(trainFile);
  return loadingHead;

}

long getTrainCount (struct loading *loadingHead, struct loading *loadingCurrent,long trainCount){
  loadingCurrent = loadingHead;
  trainCount = 1;
  while(loadingCurrent->next != NULL){
    loadingCurrent = loadingCurrent->next;
    trainCount++;
  }
  return trainCount;
}

void printWaiting (struct waiting *waitingHead, struct waiting *waitingCurrent){

  printf("Trains in Waiting Queue:\n");

  waitingCurrent = waitingHead;

  printf("%ld ",(waitingCurrent->train.id));
  printf("%c ",(waitingCurrent->train.direction));
  printf("%ld ",(waitingCurrent->train.loadTime));
  printf("%ld\n",waitingCurrent->train.crossTime);
  while(waitingCurrent->next != NULL){
    waitingCurrent = waitingCurrent->next;
    printf("%ld ",(waitingCurrent->train.id));
    printf("%c ",waitingCurrent->train.direction);
    printf("%ld ",waitingCurrent->train.loadTime);
    printf("%ld\n",waitingCurrent->train.crossTime);
  }
}

void printLoading (struct loading *loadingHead, struct loading *loadingCurrent){

  printf("Trains in Loading Queue:\n");

  loadingCurrent = loadingHead;

  printf("%ld ",(loadingCurrent->train.id));
  printf("%c ",(loadingCurrent->train.direction));
  printf("%ld ",(loadingCurrent->train.loadTime));
  printf("%ld\n",loadingCurrent->train.crossTime);
  while(loadingCurrent->next != NULL){
    loadingCurrent = loadingCurrent->next;
    printf("%ld ",(loadingCurrent->train.id));
    printf("%c ",loadingCurrent->train.direction);
    printf("%ld ",loadingCurrent->train.loadTime);
    printf("%ld\n",loadingCurrent->train.crossTime);
  }
}

struct waiting* addToWaitingQueue(struct waiting *waitingHead, struct waiting *waitingCurrent, struct Train tempTrain){

  pthread_mutex_lock (&waitingLock);

  struct waiting* waitingNew = ( struct waiting * )malloc( sizeof( struct waiting ) );
  waitingNew->train.id = tempTrain.id;
  waitingNew->train.direction = tempTrain.direction;
  waitingNew->train.loadTime = tempTrain.loadTime;
  waitingNew->train.crossTime = tempTrain.crossTime;
  waitingNew->next = NULL;

  if(waitingHead == NULL){
    waitingHead = waitingNew;
  }
  else{
    waitingCurrent = waitingHead;
    while(waitingCurrent->next!=NULL){
      waitingCurrent = waitingCurrent->next; 
    }
   waitingCurrent->next = waitingNew;
  }
  return waitingHead;
}

void* waitForTime(void* arg) //waits for the amount of seconds passed through
{ 
    long seconds;
    seconds = (long)arg;
    printf("Waiting for %ld seconds\n",seconds);
    sleep(seconds);
    printf("Waited for %ld seconds\n",seconds);
    return NULL; 
} 

int main(int argc, char *argv[]){


  //loading in the file
  char const* const fileName = argv[1]; 

  //create the head and a current node for the loading queue 
  struct loading *loadingHead = ( struct loading * )malloc( sizeof( struct loading) );
  loadingHead = NULL;
  struct loading *loadingCurrent = ( struct loading * )malloc( sizeof( struct loading ) );

  //create the head and a current node for the waiting queue 
  struct waiting *waitingHead = ( struct waiting * )malloc( sizeof( struct waiting) );
  waitingHead = NULL;
  struct waiting *waitingCurrent = ( struct waiting * )malloc( sizeof( struct waiting ) );

  loadingHead = readFile(loadingHead, loadingCurrent, fileName);

  long int trainCount = 0; //count to see how many trains are in the file
  trainCount = getTrainCount(loadingHead,loadingCurrent,trainCount);


  printf( "\n");
  printLoading(loadingHead,loadingCurrent);


  //the amount of threads for the train
  int NUM_THREADS = trainCount;

  pthread_t threads[NUM_THREADS];
  int rc;
  long t;

  loadingCurrent = loadingHead;

  t = 1;
  while(1){
    printf("In main: creating thread %ld\n", t);
    rc = pthread_create(&threads[t], NULL, waitForTime, (void *)loadingCurrent->train.loadTime);
      if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
    while(!ready){
      pthread_cond_wait (&waitingCond, &waitingLock);//wait
    }
    ready = false;
    waitingHead = addToWaitingQueue(waitingHead,waitingCurrent,loadingCurrent->train);
    pthread_cond_signal (&waitingCond);
    pthread_mutex_unlock (&waitingLock);
    ready = true;

    if(loadingCurrent->next ==NULL){
      break;
    }
    loadingCurrent = loadingCurrent->next;
    t++;
  }


  pthread_join(threads[t],NULL);

  printWaiting(waitingHead,waitingCurrent);
  


  // for(t=0;t<NUM_THREADS;t++){
  //   printf("In main: creating thread %ld\n", t);
  //     rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
  //     if (rc){
  //       printf("ERROR; return code from pthread_create() is %d\n", rc);
  //       exit(-1);
  //     }
  // }

/* Last thing that main() should do */
 pthread_exit(NULL);
}
