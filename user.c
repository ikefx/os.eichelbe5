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

#define FLAGS (O_RDWR | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int getnamed(char *name, sem_t **sem, int val);
int getRandomNumber(int low, int high);
pid_t r_wait(int * stat_loc);

int main(int argc, char * argv[]){
	/* LOAD SEMAPHORE */	
	sem_t * semaphore;
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	/* WAIT UNTIL ACCESSIBLE */
	while(sem_wait(semaphore) == -1){
		perror("Failed to block semaphore");
		return 1;
	}

	/* CRITICAL AREA */

	/* READ SHARED MEMORY */
	int fd_shm0 = shm_open("CLOCK", O_RDWR, 0666);
	unsigned long * clockPtr = mmap(0, sizeof(unsigned long)*2, PROT_WRITE, MAP_SHARED, fd_shm0, 0);
	

	* clockPtr += 5e9;
	printf("\t\t\t--> Time: %lu\n", *clockPtr);
	


	
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
	shm_unlink("CLOCK");
	exit(0);
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

int getRandomNumber(int low, int high){
	/* get random number within range */
	int num;
	for ( int i = 0; i < 2; i++ ){
		num = (rand() % (high - low + 1)) + low;
	}
	return num;
}

pid_t r_wait(int * stat_loc){
	/* a function that restarts wait if interrupted by a signal */
	int retval;
	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
	return retval;
}
