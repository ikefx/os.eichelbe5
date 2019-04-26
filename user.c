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
	bool requestedReso = false;
	bool releasedReso = false;
	unsigned long previousTime = 0;
	time_t start, stop;
	start = time(NULL);
	int pname = atoi(argv[1]);
	int max = atoi(argv[2]);
	int maxReso = atoi(argv[3]);
	sem_t * semaphore;
	/* READ SHARED MEMORY */
	int fd_shm0 = shm_open("CLOCK", O_RDWR, 0666);
	unsigned long * clockPtr = (unsigned long*)mmap(0, sizeof(unsigned long)*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm0, 0);
	/* RESOURCE REQUEST IN SHARED MEMORY */
	size_t resoSize = sizeof(int) * max;
	int fd_shm1 = shm_open("RESC", O_RDWR, 0666);
	ftruncate( fd_shm1, resoSize);
	int * resoPtr = (int*)mmap(0, resoSize, PROT_WRITE, MAP_SHARED, fd_shm1, 0);
	/* LOAD SEMAPHORE */	
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	sem_wait(semaphore);
	srand(getpid());
	int bound = getRandomNumber(1,100);
	printf("\t-- Child %2d:%5d Created -------------------------\n", pname, getpid());
	fflush(stdout);
	sem_post(semaphore);
	/* INFINITE LOOP */
	while(1){
		int diceRoll = getRandomNumber(0,100);
		/* CHANCE PROCESS REQUESTS|RELEASES A RESOURCE */
		if(diceRoll > 30 && *clockPtr > previousTime + (unsigned long)1e9/2){
			diceRoll = getRandomNumber(0,100);
			/* PROCESS REQUESTS A RESOURCE */
			if(!requestedReso && diceRoll >= 50){
				requestedReso = true;
				releasedReso = false;
				int randReso = getRandomNumber(1, maxReso);
				sem_wait(semaphore);
				writeString("output.txt", pname, *clockPtr, "child requesting resource");
				printf("\tChild :%2d:%5d requesting resource at %.0lu:%lu\n", pname, getpid(), *clockPtr/(unsigned long)1e9, *clockPtr);
				fflush(stdout);
				resoPtr[pname-1] = randReso;
				sem_post(semaphore);
			}
			/* PROCESS RELEASES ITS RESOURCE (IF ACCEPTED BY PARENT)*/
			else if(requestedReso && !releasedReso && start < start+10e9 && diceRoll <= 20) {
				requestedReso = false;
				releasedReso  = true;
				sem_wait(semaphore);
				writeString("output.txt", pname, *clockPtr, "child releasing resource");
				resoPtr[pname-1] = 0;
				printf("\tChild :%2d:%5d releasing resource at %.0lu:%lu\n", pname, getpid(), *clockPtr/(unsigned long)1e9, *clockPtr);
				fflush(stdout);
				sem_post(semaphore);
			}
			sem_wait(semaphore);
			previousTime = *clockPtr;
			sem_post(semaphore);
		}
		/* CHANCE PROCESS TERMINATES ITSELF */
		diceRoll = getRandomNumber(0,100);
		sem_wait(semaphore);
		if(*clockPtr >= (unsigned long)bound*(unsigned long)1e9 && diceRoll <= 10){	
			if(requestedReso == true) printf("\tChild :%2d:%5d releasing resource at %.0lu:%lu\n", pname, getpid(), *clockPtr/(unsigned long)1e9, *clockPtr);	
			printf("\t\t\t\t  -- Child %2d:%5d completes ----------------------\n", pname, getpid());
			fflush(stdout);
			writeOut("output.txt", pname, *clockPtr);
			resoPtr[pname-1] = 0;
			sem_post(semaphore);
			break;
		}
		sem_post(semaphore);
		/* HANDLING HUNG LOOP - SAFEGUARD TIMEOUT KILLS PROCESS IF STUCK */
		stop = time(NULL);
		if(stop - start > 30){
			printf("\t\tTimeout occured, killing %d:%d\n", pname, getpid());
			r_wait(NULL);
			sem_close(semaphore);
			shm_unlink("CLOCK");
			shm_unlink("RESC");
			shm_unlink("REQU");
			shm_unlink("PIDS");
			shm_unlink("/SEMA");
			exit(0);
		}
	}
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
