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

bool requestExceeds(int * resource, int * request);
void printTable(int * resource, int * request);
int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);
void sigintHandler(int sig_num);

/* RESOURCE TABLE */
int res[] = { 10, 2, 5, 5, 5, 2, 2, 3, 3, 1,
	4, 4, 4, 6, 6, 6, 7, 8, 8, 9 };

/* PROCESS COMPLETE BEFORE TERMINATE */
const int max = 2;

int main(int argc, char * argv[]){
	signal(SIGINT, sigintHandler);
	printf("\t\t--> OSS START <--\n\t\tParent PID: %d\n\n",getpid());
	srand(getpid());
	/* INIT VARIABLES */
	int procC = 0;
	int childPid;
	int procTotal = 0;
	unsigned long previousTime = 0;
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
	/* REQUEST IN SHARED MEMORY */	
	int fd_shm2 = shm_open("REQU", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm2, rescSize);
	int * requPtr = (int*)mmap(0, rescSize, PROT_WRITE, MAP_SHARED, fd_shm2, 0);
	for(int i = 0; i < 20; i++){
		requPtr[i] = 0;
	}
	/* KILL PID LIST IN SHARED MEMORY */
	int fd_shm3 = shm_open("PIDS", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm3, sizeof(getpid()));
	int * pidPtr = (int*)mmap(0, sizeof(getpid()), PROT_WRITE, MAP_SHARED, fd_shm3, 0);
	/* WHILE TOTAL PROCESSES < 30 OR CHILDREN INCOMPELETE 	*
 	*  INCREASE CLOCK AND POSSIBLY CREATE ANOTHER CHILD 	*/ 	
//	while(procTotal < max || (pid = wait(NULL)) > 0){
	while(procTotal < max || (pid = wait(NULL)) > 0){
		sem_wait(semaphore);
		previousTime += *clockPtr;
		sem_post(semaphore);
		if(procTotal < max && *clockPtr > 1e9){	
			if(getRandomNumber(0,100) <= 10){
				procC++;
				procTotal++;
				if(procC > 18){
						wait(NULL);
					procC--;
				}
				if((childPid = fork()) == 0){
					char buffer[128];
					char * args[] = {"./user", '\0'};
					execvp("./user", args);
				}
			}
		}
		sem_wait(semaphore);
		* clockPtr += 5e8;
		sem_post(semaphore);
		/* RUN DEADLOCK PREVENTION EVERY WHOLE SECOND */;
//		sem_wait(semaphore);
//		if(*clockPtr - previousTime >= (unsigned long)1e9/3){	
//		if(requestExceeds(rescPtr, requPtr)){
//			if(*pidPtr > 0){
//	//		printf("PARENT KILLING %d:%d\n", procC, *pidPtr);
//				kill(*pidPtr,SIGKILL);
//				printf("Parent interjects\n");
//			}
//		}
		previousTime = 0;
		printf("PARENT\n");
	}
//	while((pid = wait(NULL)) > 0 ){
		/* RUN DEADLOCK PREVENTION EVERY WHOLE SECOND */;
	//	sem_wait(semaphore);
	//	if(*clockPtr - previousTime >= (unsigned long)1e9){
	//		if(requestExceeds(rescPtr, requPtr)){
				//printf("---> PARENT DETECTS RESOURCE ERROR: 1ST LOOP\n");
				//printTable(rescPtr, requPtr);
				//if(*pidPtr > 0){
	//			printf("PARENT KILLING %d:%d\n", procC, *pidPtr);
	//			printTable(rescPtr, requPtr);
	//				kill(*pidPtr,SIGQUIT);
	//			//}
	//		}
	//		previousTime = 0;
	//	}
	//	sem_post(semaphore);
	//}
	/* STAY IN LOOP WHILE CHILDREN RUNNING */
//	while((pid = wait(NULL)) > 0){
		/* CONTINUE DEADLOCK MONITORING */
//		sem_wait(semaphore);
//		if(*clockPtr - previousTime >= (unsigned long)1e9/5){
//			if(requestExceeds(rescPtr, requPtr)){
//				printf("---> PARENT DETECTS RESOURCE ERROR: 2ND LOOP\n");
//				printTable(rescPtr, requPtr);
//				printf("KILLING %d\n", getpid());
//				kill(pid, SIGKILL);
//			}
//			previousTime = 0;
//		}
//		*clockPtr += 5e8;
//		previousTime += *clockPtr;
//		sem_post(semaphore);
//	}
	printf("\nOSS: %d Processes total were made.\n", procTotal);
	printf("\t\t--> OSS Terminated at %f <--\n", * clockPtr / 1e9);
	sem_unlink("/SEMA");
	sem_destroy(semaphore);
	shmdt(requPtr);
	shmdt(rescPtr);
	shmdt(pidPtr);
	sem_unlink("CLOCK");
	sem_unlink("RESC");
	sem_unlink("REQU");
	sem_unlink("PIDS");
	return 0;
}

bool requestExceeds(int * resource, int * request){
	/* BOOL RETURNS T/F IF REQUEST EXCEEDS RESOURCE */
	for(int i = 0; i < 20; i++){
		if(request[i] > resource[i]){
			return true;
		}
	}
	return false;
}

void printTable(int * resource, int * request){
	printf("\tTABLE FROM PARENT\n");
	printf("\t---------- RESOURCE TABLE ----------\n\t");
	for(int i = 0; i < 20; i++){
		printf("%2d ", resource[i]);
	}
	printf("\n\t");
	for(int i = 0; i < 20; i++){
		printf("%2d ", request[i]);
	}
	printf("\n\t---------- -------------- ----------\n");
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

void sigintHandler(int sig_num){
	/* CTRL C KILL */
	signal(SIGINT, sigintHandler);
	printf("\nTerminating all...\n");
	shm_unlink("CLOCK");
	shm_unlink("RESC");
	shm_unlink("/SEMA");
	kill(0,SIGTERM);
	exit(0);
}
