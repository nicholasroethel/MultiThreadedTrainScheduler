#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t waitingLock =  PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t waitingCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t dispatchLock =  PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t dispatchCond = PTHREAD_COND_INITIALIZER;

bool ready = true;
bool dispatchReady = true;
bool track = false;

long int west = 0;
long int east = 0; 
int trainsWaiting = 0;
int trainsSent = 0;

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

//create the head and a current node for the waiting queue 
struct waiting *waitingHead = NULL;
struct waiting *waitingCurrent = NULL;

void *PrintHello(void *threadid)
{
 long tid;
 tid = (long)threadid;
 printf("Hello World! It's me, thread #%ld!\n", tid);
 pthread_exit(NULL);
}


//adds a train to the loading queue
struct loading* addToLoadingQueue(struct loading *loadingHead, struct loading *loadingCurrent, struct Train tempTrain){ 

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


//reads file and puts trains in the loading queue
struct loading* readFile(struct loading *loadingHead,  struct loading *loadingCurrent, char const* const fileName){

  FILE* trainFile = fopen(fileName, "r"); 
  char line[256];

  //setup for tokenization
  const char delim[2] = " ";
  char *token;

  //printf("Trains Read:\n");

  long trainCount = 1;
  //iterate through the file and create the trains
  while (fgets(line, sizeof(line), trainFile) != NULL) {

    //allocate memory for the train
    struct Train *tempTrain = ( struct Train * )malloc( sizeof( struct Train ) );

    tempTrain->id = trainCount-1;
    //printf("%ld ",trainCount);

    //get the trains direction
    token = strtok(line, delim);
    tempTrain->direction = *token;
    //printf( "%s ", token );

    //get the trains load time
    token = strtok(NULL, delim);
    tempTrain->loadTime = atoi(token);
    //printf( "%s ", token );

    //get the trains crossing time
    token = strtok(NULL, delim);
    tempTrain->crossTime = atoi(token);
    //printf( "%s", token ); 

    loadingHead = addToLoadingQueue(loadingHead,loadingCurrent,*tempTrain);
    trainCount++;
  }
  //close the file
  fclose(trainFile);
  return loadingHead;

}


//gets the number of trains
long getTrainCount (struct loading *loadingHead, struct loading *loadingCurrent,long trainCount){
  loadingCurrent = loadingHead;
  trainCount = 1;
  while(loadingCurrent->next != NULL){
    loadingCurrent = loadingCurrent->next;
    trainCount++;
  }
  return trainCount;
}

//prints the trains in the waiting queue
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

//prints the trains in the loading queue
void printLoading (struct loading *loadingHead, struct loading *loadingCurrent){

  printf( "\n");
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

//adds trains to the waiting queue
void * addToWaitingQueue(void* arg){
  struct Train* tempTrain = (struct Train*)arg;
  sleep(tempTrain->loadTime);
  //printf("slept for: %ld\n",tempTrain->loadTime);

  while(!ready){
    pthread_cond_wait (&waitingCond, &waitingLock);//wait
  }
  ready = false;
  pthread_mutex_lock (&waitingLock);

  struct waiting* waitingNew = ( struct waiting * )malloc( sizeof( struct waiting ) );
  waitingNew->train.id = tempTrain->id;
  waitingNew->train.direction = tempTrain->direction;
  waitingNew->train.loadTime = tempTrain->loadTime;
  waitingNew->train.crossTime = tempTrain->crossTime;
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

  pthread_cond_signal (&waitingCond);
  pthread_mutex_unlock (&waitingLock);
  trainsWaiting++;
  ready = true;

  return NULL;
}

long int dispatcher(struct waiting *waitingHead, struct waiting *waitingCurrent, long int trainCount, bool dispatch[]){
  long int minID = trainCount; 
  long int currentBestID;
  long int currentLowestLoadingTime; 
  long int currentCrossTime = 0;
  char currentBestPriority = 'n';
  int shouldSwap = 0;

  waitingCurrent = waitingHead;

  if (waitingCurrent->next==NULL && dispatch[0] == false){//if dispatching the first train
      currentBestID = waitingCurrent->train.id; 
      currentCrossTime = waitingCurrent->train.crossTime;
  }
  else{ //if dispatching something else
    while(waitingCurrent->next != NULL){
      if(dispatch[waitingCurrent->train.id] == false){ //if it hasn't been dispatched yet
        if((waitingCurrent->train.direction == 'e'|| waitingCurrent->train.direction == 'w') && (currentBestPriority == 'E'|| currentBestPriority == 'W')){
          shouldSwap = 1; 
        }
        else if((waitingCurrent->train.direction == 'e'|| waitingCurrent->train.direction =='w') && (currentBestPriority == 'e'|| currentBestPriority == 'w')){
          if(waitingCurrent->train.loadTime<currentLowestLoadingTime){
            shouldSwap = 1; 
          }
          else if(waitingCurrent->train.loadTime==currentLowestLoadingTime){
            if(waitingCurrent->train.id<currentBestID){
              shouldSwap = 1;   
            }
          }
        }
        else if(currentBestPriority == 'n'){
          shouldSwap = 1;
        }
      }
      if(shouldSwap == 1){
        currentBestID = waitingCurrent->train.id; 
        currentBestPriority = waitingCurrent->train.direction;
        currentLowestLoadingTime = waitingCurrent->train.loadTime;
        currentCrossTime = waitingCurrent->train.crossTime;
        shouldSwap = 0;
      }
      waitingCurrent = waitingCurrent->next;
    }
  }

  track = true;
  sleep(currentCrossTime);
  track = false;
  trainsWaiting --; 
  return currentBestID;
}

int main(int argc, char *argv[]){

  //loading in the file
  char const* const fileName = argv[1]; 

  //create the head and a current node for the loading queue 
  struct loading *loadingHead = ( struct loading * )malloc( sizeof( struct loading) );
  loadingHead = NULL;
  struct loading *loadingCurrent = ( struct loading * )malloc( sizeof( struct loading ) );
  loadingCurrent = NULL;

  //read in file and get the head of the loading list
  loadingHead = readFile(loadingHead, loadingCurrent, fileName);

  //get the amount of trains
  long int trainCount = getTrainCount(loadingHead,loadingCurrent,trainCount);
  long int trainsLeft = trainCount;

  printLoading(loadingHead,loadingCurrent);

  //the amount of threads for the train
  int NUM_THREADS = trainCount;

  bool dispatch[trainCount];
  long int done;

  pthread_t threads[NUM_THREADS];
  int rc;
  long t;

  loadingCurrent = loadingHead;

  t = 1; 
  while(1){
    printf("In main: creating thread %ld\n", t);
    rc = pthread_create(&threads[t], NULL, addToWaitingQueue, &(loadingCurrent->train));
    if (rc){
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
    //printWaiting(waitingHead,waitingCurrent);

    if(loadingCurrent->next ==NULL){
      break;
    }
    loadingCurrent = loadingCurrent->next;
    t++;
  }

  while(trainsLeft>0){
      while(trainsWaiting>0 && track == false){
          while(!dispatchReady){
            pthread_cond_wait (&dispatchCond, &dispatchLock);//wait
          }
        dispatchReady = false;
        pthread_mutex_lock (&dispatchLock);
        done = dispatcher(waitingHead, waitingCurrent, trainCount, dispatch);
        if(done<trainCount){
          printf("Dispatching: %ld\n", done);
          dispatch[done] = true;
          trainsLeft--;
        }
        pthread_cond_signal (&dispatchCond);
        pthread_mutex_unlock (&dispatchLock);
        dispatchReady = true;
        

      }
  }

    //pthread_join(threads[t],NULL);


  pthread_exit(NULL);
}
