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

void writeString(char * name, int child, unsigned long time, char * string);
void writeOut(char * name, int child, unsigned long time);
void printTable(int * resource, int * request);
int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);
pid_t r_wait(int * stat_loc);

int main(int argc, char * argv[]){
	/* INIT VARIABLES */
	int differentResourcesNeeded = 0;
	bool requestedReso = false;
	bool releasedReso = false;
	bool isWaiting = false;
	bool hadToWait = false;
	int pname = atoi(argv[1]);
	sem_t * semaphore;
	/* READ SHARED MEMORY */
	int fd_shm0 = shm_open("CLOCK", O_RDWR, 0666);
	unsigned long * clockPtr = (unsigned long*)mmap(0, sizeof(unsigned long)*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm0, 0);
	int fd_shm1 = shm_open("RESC", O_RDWR, 0666);
	int * rescPtr = (int*)mmap(0, sizeof(int)*20, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm1, 0);
	int fd_shm2 = shm_open("REQU", O_RDWR, 0666);
	int * requPtr = (int*)mmap(0, sizeof(int)*20, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm2, 0);
	int fd_shm3 = shm_open("PIDS", O_RDWR, 0666);
	int * pidsPtr = (int*)mmap(0, sizeof(getpid()), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm3, 0);
	/* LOAD SEMAPHORE */	
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	sem_wait(semaphore);
	unsigned long start = *clockPtr;
	sem_post(semaphore);

	srand(getpid()*time(NULL));
	int bound = getRandomNumber(0,100);
	printf("\t-- Child %d:%d Created -------------------------\n", pname, getpid());
	while(1){
		srand(getpid()*time(NULL)* *clockPtr);
		int diceRoll = getRandomNumber(0,100);
		/* CHANCE PROCESS REQUESTS|RELEASES A RESOURCE */
		if(diceRoll > 80){
			int reqrelroll = getRandomNumber(0,100);
			/* PROCESS REQUESTS A RESOURCE */
			if(!requestedReso && reqrelroll >= 50){
				writeString("output.txt", pname, *clockPtr, "child requesting resource");
				requestedReso = true;
			}
			/* PROCESS RELEASES ITS RESOURCE (IF ACCEPTED BY PARENT)*/
			if(requestedReso && reqrelroll < 50) {
				writeString("output.txt", pname, *clockPtr, "child releasing resource");
				requestedReso = false;
				releasedReso  = true;
			}
		}
		/* CHANCE PROCESS TERMINATES ITSELF */
		if(releasedReso && *clockPtr >= (unsigned long)bound*(unsigned long)1e9){
			break;
		}
		*clockPtr += 5e8;
	}
//	sem_wait(semaphore);
	printf("\t\t\t\t-- Child %d completes ----------------------\n", getpid());
	writeOut("output.txt", pname, *clockPtr);
//	sem_post(semaphore);
	if(r_wait(NULL) == -1){
		return 1;
	}
	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");
	}
	shm_unlink("CLOCK");
	shm_unlink("RESC");
	shm_unlink("REQU");
	shm_unlink("PIDS");
	shm_unlink("/SEMA");
	exit(0);
}

void writeString(char * name, int child, unsigned long time, char * string){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "\t\t\tChild %d:%d\n\t\t\t\t%s\n\t\t\t\t\tat %.0lu:%lu\n", child, getpid(), string, time/(unsigned long)1e9, time);
	fprintf(fp, wroteline);
	fclose(fp);
	return;
}
void writeOut(char * name, int child, unsigned long time){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "\t\t\tChild %d:%d terminated at %.0lu:%lu\n", child, getpid(), time/(unsigned long)1e9, time);
	fprintf(fp, wroteline);
	fclose(fp);
	return;
}
void printTable(int * resource, int * request){
	printf("\t\t---------- RESOURCE TABLE ----------\n\t\t");
	for(int i = 0; i < 20; i++){
		printf("%2d ", resource[i]);
	}
	printf("\n\t\t");
	for(int i = 0; i < 20; i++){
		printf("%2d ", request[i]);
	}
	printf("\n\t\t---------- -------------- ----------\n");
	fflush(stdout);
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
