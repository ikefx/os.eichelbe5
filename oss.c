/* NEIL EICHELBERGER
 * cs4760 assignment 5
 * OSS file */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);

/* RESOURCE TABLE */
int res[] = { 10, 2, 5, 5, 5, 2, 2, 3, 3, 1,
	4, 4, 4, 6, 6, 6, 7, 8, 8, 9 };

int main(int argc, char * argv[]){
	/* INIT VARIABLES */
	int procC = 0;
	int procTotal = 0;
	pid_t pid;
	/* SEMAPHORE */
	sem_t * semaphore;
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	/* TIMER IN SHARED MEMORY */
	size_t clockSize = sizeof(unsigned long) * 2;
	int fd_shm0 = shm_open("CLOCK", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm0, clockSize );
	unsigned long * clockPtr = (unsigned long*)mmap(0, clockSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm0, 0);
	* clockPtr = 0;
	/* RESOURCES IN SHARED MEMORY */
	size_t rescSize = sizeof(int) * 20;
	int fd_shm1 = shm_open("RESC", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm1, rescSize);
	int * rescPtr = (int*)mmap(0, rescSize, PROT_WRITE, MAP_SHARED, fd_shm1, 0);
	for(int i = 0; i < 20; i++){
		rescPtr[i] = res[i];
	}	

	while(procTotal < 30){
		if(getRandomNumber(0,100) <= 33){
			procC++;
			procTotal++;
			if(procC > 18){
				wait(NULL);
				procC--;
			}
			if((pid = fork()) == 0){
				char * args[] = {"./user", '\0'};
				execvp("./user", args);
			}
		}
	}
	while((pid = wait(NULL)) > 0);
	sem_unlink("/SEMA");
	sem_destroy(semaphore);
	sem_unlink("CLOCK");
	return 0;
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
