#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

const int DEER = 9; /* constant that controls the number of reindeer */
const int ELVES = 20; /* constant that controls the number of elves */

pthread_mutex_t santaLock; /* lock for access to santa */
int santaDoor = 0; /* int that keeps track of elves waiting at Santa's door (only 3 allowed) */
pthread_mutex_t elfQueMutex; /* mutex to control the access to the santaDoor int */
sem_t wakeSanta; /* Semaphore that santa will watch and wake up with a signal */
sem_t helpQue; /*semaphore to keep track of elves while santa helps each one */
sem_t lastElf; /* semaphore to make sure the last elf gets helped by santa and no-one jumps in line */

int deerInStable = 0; /* keep track of deer waiting to be attached to sleigh */
sem_t deerWaiting; /* wait to be attached */
bool deerReady = false; /* barrier to wait until all deer are attached before going to deliver presents */
sem_t deerFinished; /* making sure all deer are finished before santa returns home */
pthread_mutex_t deerMutex; /* mutex to control access to the deerInStable int */


/* elf gets help from santa */
void getHelp(){
  printf("elf (%lu) got help from Santa\n", pthread_self() % 100);
}


void* elf(void* arg){
  while(true){
    int workDuration = rand() % 100; /* get a number between 0 and 99 */
    printf("elf (%lu) is working for %d\n", pthread_self() % 100, workDuration);
    sleep(workDuration); /* elf works for a random amount of time and then runs into a problem */
    pthread_mutex_lock(&santaLock); /* lock santa so santaDoor and  deerInStable can't be changed at the same time so Santa will always know who got to him first */
    printf("elf (%lu) has a problem and goes to Santa for help\n", pthread_self() % 100);
    pthread_mutex_lock(&elfQueMutex);
    santaDoor++;
    if(santaDoor == 3){ /* only the third elf will trigger this */
      sem_post(&wakeSanta); /* wake Santa */
      sem_wait(&lastElf); /* wait until santa finishes helping the other two elves */
      sem_wait(&helpQue); /* elf gets help from santa */
      getHelp();
      santaDoor -= 3; /* return santaDoor to 0 */
      pthread_mutex_unlock(&santaLock);
      pthread_mutex_unlock(&elfQueMutex); /* no elves can get in que until the last elf is done with Santa */
    }else{
      pthread_mutex_unlock(&santaLock); /* release the locks since this is not the last elf */
      pthread_mutex_unlock(&elfQueMutex);
      sem_wait(&helpQue); /* elf gets help from santa */
      getHelp();
    }
  }
}

void deliverPresents(){
  pthread_mutex_lock(&deerMutex);
  deerInStable--;
  printf("reindeer (%lu) helped deliver presents\n", pthread_self() % 100);
  pthread_mutex_unlock(&deerMutex);
  sem_post(&deerFinished);
  /* when printing the thread id we only take the first 2 digits because it is unlikely that they are the duplicated */
  /* the id does not look nice when printed in full */
}

void* deer(void* arg){
  while(true){
    int vacationTime = rand() % 50; /* take a vaccation for 0 to 19 seconds */
    printf("reindeer (%lu) is vacationing for %d\n", pthread_self() % 100, vacationTime);
    sleep(vacationTime);
    pthread_mutex_lock(&santaLock); /* lock santa so deerInStable and santaDoor can't change at the same time */
    pthread_mutex_lock(&deerMutex);
    deerInStable++;
    if(deerInStable == DEER){
      printf("reindeer (%lu) arrived to the north pole and will wake up Santa\n", pthread_self() % 100);
      sem_post(&wakeSanta); /* wake up santa */
      pthread_mutex_unlock(&deerMutex); /* unlock deermutex so each deer can remove themselves from stable */
      sem_wait(&deerWaiting); /* deer waiting to be attached to sleigh */
      while(!deerReady); /* waiting here until all deer are attached */
      deliverPresents(); 
      pthread_mutex_unlock(&santaLock); /* releasing santa after all presents are delivered */
    }else{
      printf("reindeer (%lu) arrived to the north pole and is waiting for Santa\n", pthread_self() % 100);
      pthread_mutex_unlock(&santaLock); /* releasing locks since this is not the last deer */
      pthread_mutex_unlock(&deerMutex);
      sem_wait(&deerWaiting); /* waiting to be attached to sleigh */
      while(!deerReady); /* waiting for all deer to be attached then delivering presents */
      deliverPresents();
    }
  }
}

void* santa(void* arg){
  while(true){
    printf("Santa is sleeping in his office\n");
    sem_wait(&wakeSanta); /* wait to be awakened by the reindeer or the elves */
    if(deerInStable == DEER){ /* check who woke santa up, it can only be dear or elves, not both, since we lock santa when changing either deerInStable or santaDoor */
      for(int i = 0; i <= DEER; i++){
	sem_post(&deerWaiting); /* attach all deer to sleigh */
	printf("Santa fastened a deer to the sleigh\n");
      }
      printf("Santa fastened all the deer and will deliver the presents\n");
      deerReady = true; /* after attaching all deer, flip the barrier flag */
      for(int i = 0; i < DEER; i++){
	sem_wait(&deerFinished); /* wait until all deer help deliver presents and then go back to northpole after */
      }
      deerReady = false; /* flip the barrier for next loop */
      printf("Santa delivered all presents and returned to the north pole\n");
    }else if(santaDoor == 3){
      sem_post(&helpQue); /* if it was the elves that woke santa, help all three elves */
      printf("Santa helped an elf\n");
      sem_post(&helpQue);
      printf("Santa helped an elf\n");
      sem_post(&lastElf); /* signal to the last elf that the other two finished then help it */
      sem_post(&helpQue);
      printf("Santa helped three elves and closed his door\n");
    }
  }
}

int main(){
  /* initializing our mutexes and semaphores */
  pthread_mutex_init(&santaLock, NULL);
  sem_init(&wakeSanta, 0, 0);
  sem_init(&helpQue, 0, 0);
  pthread_mutex_init(&elfQueMutex, NULL);
  sem_init(&lastElf, 0, 0);
  pthread_mutex_init(&deerMutex, NULL);
  sem_init(&deerWaiting, 0, 0);
  sem_init(&deerFinished, 0, 0);

  /* create a thread for each deer */
  pthread_t deerThreads[DEER];
  for(int i = 0; i < DEER; i++){
    pthread_create(&deerThreads[i], NULL, deer, NULL);
  }

  /* create the Santa thread */
  pthread_t santaThread;
  pthread_create(&santaThread, NULL, santa, NULL);

  /* create a thread for each elf */
  pthread_t elfThreads[ELVES];
  for(int i = 0; i < ELVES; i++){
    pthread_create(&elfThreads[i], NULL, elf, NULL);
  }
  
  /* thread join is not needed since all threads run in an infinite loop and will never return, so we only have one to make the program not return */
  pthread_join(santaThread, NULL);

  return 0;
}
