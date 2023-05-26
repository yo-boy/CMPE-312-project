#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

const int DEER = 9;
const int ELVES = 10;

pthread_mutex_t santaLock; /* lock for access to santa */
int santaDoor = 0; /* int that keeps track of elves waiting at Santa's door (only 3 allowed) */
pthread_mutex_t elfQueMutex; /* mutex to control the access to the santaDoor int */
sem_t wakeSanta; /* Semaphore that santa will watch and wake up with a signal */
sem_t helpQue; /*semaphore to keep track of elves while santa helps each one */
sem_t lastElf; /* semaphore to make sure the last elf gets helped by santa and no-one jumps in line */

int deerInStable = 0;
sem_t deerWaiting;
bool deerReady = false;
sem_t deerFinished;
pthread_mutex_t deerMutex; /* mutex to control access to the deer int */


void getHelp(){
  printf("elf (%lu) got help from Santa\n", pthread_self());
}


void* elf(void* arg){
  while(true){
    int workDuration = rand() % 10; /* get a number between 0 and 9 */
    printf("elf (%lu) is working for %d\n", pthread_self(), workDuration);
    sleep(workDuration); /* elf works for a random amount of time and then runs into a problem */
    pthread_mutex_lock(&santaLock);
    printf("elf (%lu) has a problem and goes to Santa for help\n", pthread_self(),santaDoor);
    pthread_mutex_lock(&elfQueMutex);
    santaDoor++;
    if(santaDoor == 3){
      sem_post(&wakeSanta); /* wake Santa */
      sem_wait(&lastElf); /* wait until santa finishes helping the other two elves */
      sem_wait(&helpQue); /* elf gets help from santa */
      getHelp();
      santaDoor -= 3;
      pthread_mutex_unlock(&santaLock);
      pthread_mutex_unlock(&elfQueMutex);
    }else{
      pthread_mutex_unlock(&santaLock);
      pthread_mutex_unlock(&elfQueMutex);
      sem_wait(&helpQue); /* elf gets help from santa */
      getHelp();
    }
  }
}

void deliverPresents(){
  pthread_mutex_lock(&deerMutex);
  deerInStable--;
  pthread_mutex_unlock(&deerMutex);
  sem_post(&deerFinished);
  printf("reindeer (%lu) helped deliver presents\n", pthread_self());
}

void* deer(void* arg){
  while(true){
    int vacationTime = rand() % 10; /* take a vaccation for 0 to 19 seconds */
    printf("reindeer (%lu) is vacationing for %d\n", pthread_self(), vacationTime);
    sleep(vacationTime);
    pthread_mutex_lock(&santaLock);
    pthread_mutex_lock(&deerMutex);
    deerInStable++;
    if(deerInStable == DEER){
      printf("reindeer (%lu) arrived to the north pole and will wake up Santa\n", pthread_self());
      sem_post(&wakeSanta);
      pthread_mutex_unlock(&deerMutex);
      sem_wait(&deerWaiting);
      while(!deerReady);
      deliverPresents();
      pthread_mutex_unlock(&santaLock);
    }else{
      printf("reindeer (%lu) arrived to the north pole and is waiting for Santa\n", pthread_self());
      pthread_mutex_unlock(&santaLock);
      pthread_mutex_unlock(&deerMutex);
      sem_wait(&deerWaiting);
      while(!deerReady);
      deliverPresents();
    }
  }
}

void* santa(void* arg){
  while(true){
    printf("Santa is sleeping in his office\n");
    sem_wait(&wakeSanta);
    if(deerInStable == DEER){
      for(int i = 0; i <= DEER; i++){
	sem_post(&deerWaiting);
	printf("Santa fastened a deer to the sleigh\n");
      }
      printf("Santa fastened all the deer and will deliver the presents\n");
      deerReady = true;
      for(int i = 0; i < DEER; i++){
	sem_wait(&deerFinished);
      }
      deerReady = false;
      printf("Santa delivered all presents and returned to the north pole\n");
    }else if(santaDoor == 3){
	sem_post(&helpQue);
	printf("Santa helped an elf\n");
	sem_post(&helpQue);
	printf("Santa helped an elf\n");
	sem_post(&lastElf);
	sem_post(&helpQue);
	printf("Santa helped three elves and closed his door\n");
    }
  }
}

int main(){
  pthread_mutex_init(&santaLock, NULL);
  sem_init(&wakeSanta, 0, 0);
  sem_init(&helpQue, 0, 0);
  pthread_mutex_init(&elfQueMutex, NULL);
  sem_init(&lastElf, 0, 0);
  pthread_mutex_init(&deerMutex, NULL);
  sem_init(&deerWaiting, 0, 0);
  sem_init(&deerFinished, 0, 0);

  pthread_t deerThreads[DEER];
  for(int i = 0; i < DEER; i++){
    pthread_create(&deerThreads[i], NULL, deer, NULL);
  }

  pthread_t santaThread;
  pthread_create(&santaThread, NULL, santa, NULL);

  pthread_t elfThreads[ELVES];
  for(int i = 0; i < ELVES; i++){
    pthread_create(&elfThreads[i], NULL, elf, NULL);
  }

  pthread_join(santaThread, NULL);
  
  for(int i = 0; i < ELVES; i++){
    pthread_join(elfThreads[i], NULL);
  }
  for(int i = 0; i < DEER; i++){
    pthread_join(deerThreads[i], NULL);
  }
  
  return 0;
}







