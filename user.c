/* NEIL EICHELBERGER
 * cs4760 assignment 5
 * user.c file */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>

//#define FLAGS (O_RDWR | O_EXCL)
#define FLAGS O_RDWR
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);
pid_t r_wait(int * stat_loc);
void sigintHandler(int sig_num);

int main(int argc, char * argv[]){
	signal(SIGINT, sigintHandler);
	srand(getpid());
	printf("-- Child begins -------------------------\n");
	/* INIT VARIABLES */
	int differentResourcesNeeded = 0;
	sem_t * semaphore;
	/* READ SHARED MEMORY */
	int fd_shm0 = shm_open("CLOCK", O_RDWR, 0666);
	unsigned long * clockPtr = (unsigned long*)mmap(0, sizeof(unsigned long)*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm0, 0);
	int fd_shm1 = shm_open("RESC", O_RDWR, 0666);
	int * rescPtr = (int*)mmap(0, sizeof(int)*21, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm1, 0);
	/* RANDOMLY DETERMINE RESOURCES REQUIRED */
	int requestedResources[20];
	for(int i = 0; i < 20; i++){
		requestedResources[i] = 0;
		if(getRandomNumber(0,100) > 75){
			differentResourcesNeeded++;
			requestedResources[i] = getRandomNumber(1,rescPtr[i]);
		}
	}

	/* LOAD SEMAPHORE */	
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	/* WAIT UNTIL ACCESSIBLE */
	while(sem_wait(semaphore) == -1){
		perror("Failed to block semaphore");
		return 1;
	}

	/* REQUEST RESOURCES: SLEEP IF UNAVAILABLE */
	for(int i = 0; i < 20; i++){
		fflush(stdout);
		if(requestedResources[i] > 0){
			int temp = rescPtr[i];
			printf("\t[R:%d] has %d available, requesting %d\t", i, temp, requestedResources[i]);
			int newResult = temp - requestedResources[i];
			if(newResult < 0){
				printf("RESOURCE ERROR SLEEP --> %d\n", newResult);
			}else{
				printf("ATTEMPT COMPLETE THEN RELEASE--> %d\n", newResult);
				temp -= requestedResources[i];
	//			rescPtr[i] = temp;
			}	
		}
	}	
	printf("-- Child completes ----------------------\n");
	
	/* EXIT CRITICAL AREA*/
	/* RELEASE SEMAPHORE */
	while(sem_post(semaphore) == -1){
		perror("failed to unlock semaphore");
		return 1;
	}
		
	if(r_wait(NULL) == -1){
		return 1;
	}
	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");
	}

	printf("\tCHILD IS FINISHED\n");
	shm_unlink("CLOCK");
	shm_unlink("RESC");
	shm_unlink("/SEMA");
	exit(0);
}


int getRandomNumber(int low, int high){
	/* get random number within range */
	int num;
	for ( int i = 0; i < 2; i++ ){
		num = (rand() % (high - low + 1)) + low;
	}
	return num;
}

int getnamed(char *name, sem_t **sem, int val){
	/* a function to access a named seamphore, creating it if it dosn't already exist */
	while(((*sem = sem_open(name, FLAGS , PERMS , val)) == SEM_FAILED) && (errno == EINTR));
	if(*sem != SEM_FAILED)
		return 0;
	if(errno != EEXIST)
		return -1;
	while(((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR));
	if(*sem != SEM_FAILED)
		return 0;
	return -1;	
}

pid_t r_wait(int * stat_loc){
	/* a function that restarts wait if interrupted by a signal */
	int retval;
	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
	return retval;
}

void sigintHandler(int sig_num){
	/* CTRL C KILL */
	signal(SIGINT, sigintHandler);
	printf("\nTerminating all...\n");
	kill(0,SIGTERM);
	shm_unlink("CLOCK");
	shm_unlink("RESC");
	shm_unlink("/SEMA");
	exit(0);
}
