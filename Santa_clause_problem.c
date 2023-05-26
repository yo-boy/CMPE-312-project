#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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
pthread_mutex_t deerMutex; /* mutex to control access to the deer int */


void getHelp(){
  printf("elf (%lu) got help from Santa", pthread_self());
}


void* elf(){
  while(true){
    int workDuration = rand() % 10; /* get a number between 0 and 9 */
    printf("elf (%lu) is working for %d\n", pthread_self(), workDuration);
    sleep(workDuration); /* elf works for a random amount of time and then runs into a problem */
    pthread_mutex_lock(&elfQueMutex);
    santaDoor++;
    if(santaDoor == 3){
      santaDoor -= 3;
      pthread_mutex_lock(&santaLock);
      sem_post(&wakeSanta); /* wake Santa */
      sem_wait(&lastElf); /* wait until santa finishes helping the other two elves */
      sem_wait(&helpQue); /* elf gets help from santa */
      getHelp();
      pthread_mutex_unlock(&santaLock);
      pthread_mutex_unlock(&elfQueMutex);
    }else{
      pthread_mutex_unlock(&elfQueMutex);
      sem_wait(&helpQue); /* elf gets help from santa */
      getHelp();
    }
  }
}

void deliverPresents(){
  pthread_mutex_lock(&deerMutex);
  deerWaiting--;
  pthread_mutex_unlock(&deerMutex);
  printf("reindeer (%lu) helped deliver presents", pthread_self());
}

void* deer(){
  while(true){
    int vacationTime = rand() % 20; /* take a vaccation for 0 to 19 seconds */
    printf("reindeer (%lu) is vacationing for %d\n", pthread_self(), vacationTime);
    sleep(vacationTime);
    pthread_mutex_lock(&deerMutex);
    deerInStable++;
    if(deerInStable == DEER){
      pthread_mutex_lock(&santaLock);
      sem_post(&wakeSanta);
      pthread_mutex_unlock(&deerMutex);
      wait(&deerWaiting);
      deliverPresents();
      
    }else{
      pthread_mutex_unlock(&deerMutex);
      wait(&deerWaiting);
      deliverPresents();
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
  /*sem_getvalue(&santaDoor, &num); */
  return 0;
}







