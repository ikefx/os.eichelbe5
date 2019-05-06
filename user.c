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

#define max 50
#define SHM_KEY 0x9893
#define FLAGS O_RDWR
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

struct rTable {
	int request[max];
	int approved[max];
	int resource[max];
	int requestC;
	int releaseC;
};

const size_t SHMSZ = sizeof(struct rTable);
struct rTable * rptr;
int shm1;

void writeRequest(char * name, int child, unsigned long time, char * string);
void writeOutDeadlock(char * name, int child, unsigned long time);
void writeStringSuccess(char * name, int child, unsigned long time, char * string);
void writeString(char * name, int child, unsigned long time, char * string);
void writeOut(char * name, int child, unsigned long time);
void printTable(int * resource, int * request);
int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);
pid_t r_wait(int * stat_loc);

char * sema = "SEMA5";

int main(int argc, char * argv[]){
	/* INIT VARIABLES */
	bool requestedReso = false;
	bool releasedReso = false;
	unsigned long previousTime = 0;
	int mypid = getpid();
	time_t start, stop;
	start = time(NULL);
	int pname = atoi(argv[1]);
	int maxReso = atoi(argv[2]);
	size_t clockSize = sizeof(unsigned long) + 1;
	size_t resoSize  = sizeof(int) * max;
	sem_t * semaphore;
	/* READ SHARED MEMORY */
	int fd_shm0 = shm_open("CLOCK", O_RDWR, 0666);
	ftruncate( fd_shm0, clockSize);
	unsigned long * clockPtr = (unsigned long*)mmap(0, clockSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm0, 0);
	/* RESOURCE REQUEST IN SHARED MEMORY */
	int fd_shm1 = shm_open("RESC", O_RDWR, 0666);
	ftruncate( fd_shm1, resoSize);
	int * resoPtr = (int*)mmap(0, resoSize, PROT_WRITE, MAP_SHARED, fd_shm1, 0);
	/* USER PID LIST FROM SHARED MEMORY */
	int fd_shm2 = shm_open("PIDS", O_RDWR, 0666);
	ftruncate( fd_shm2, resoSize);
	int * pidPtr  = (int*)mmap(0, resoSize, PROT_WRITE, MAP_SHARED, fd_shm2, 0);
	/* LOAD SEMAPHORE */	
	if(getnamed(sema, &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}

	/* LOCATE SEGMENT */
	if((shm1 = shmget(SHM_KEY, SHMSZ, 0666)) < 0){
		perror("Shared memory create: shmget()");
		exit(1);}
	if((rptr = shmat(shm1, NULL, 0)) == (void*) -1){
		perror("Shared memory attach: shmat()");
		exit(1);}

	sem_wait(semaphore);
	printf("-------- Child %2d:%5d Created ---------------------\n", pname, getpid());
	fflush(stdout);
	pidPtr[pname-1] = mypid;
	usleep(100000);
	sem_post(semaphore);
	srand(mypid);
	int bound = getRandomNumber(1,160);
	/* INFINITE LOOP */
	while(1){
		/* IF SIGNAL RECEIVED FROM PARENT TO TERMINATE */
		if(pidPtr[pname-1] == 0){
			sem_wait(semaphore);
			printf("\n>>> OSS Instructed %d:%d to terminate due to deadlock prevention.\n\n", pname, getpid());
			writeOutDeadlock("output.txt", pname, *clockPtr);
			sem_post(semaphore);
			break;
		}

		/* CHANCE PROCESS REQUESTS|RELEASES A RESOURCE */
		int diceRoll = getRandomNumber(0,100);
		if(diceRoll > 30 && *clockPtr > previousTime + (unsigned long)1e9/2){
			diceRoll = getRandomNumber(0,100);
			/* PROCESS REQUESTS A RESOURCE */
			if(!requestedReso && diceRoll >= 6){
				requestedReso = true;
				releasedReso = false;
				int randReso = getRandomNumber(1, maxReso);
				sem_wait(semaphore);
				writeRequest("output.txt", pname, *clockPtr, "REQUESTING");
				printf("\t Child %2d:%5d requesting resource at %.0lu:%lu\n", pname, getpid(), *clockPtr/(unsigned long)1e9, *clockPtr);
				resoPtr[pname-1] = randReso;
				rptr->requestC++;
				sem_post(semaphore);
				/* SLEEP THE PROCESS TO SIMULATE REQUEST WAIT TIME */
				rptr->request[pname-1] = randReso;
				usleep(250000);
				if(rptr->approved[pname-1] == 0) usleep(1000000);
			}
			/* PROCESS RELEASES ITS RESOURCE (IF ACCEPTED BY PARENT)*/
			else if(requestedReso && !releasedReso) {
				requestedReso = false;
				releasedReso  = true;
				sem_wait(semaphore);
				resoPtr[pname-1] = 0;
				rptr->releaseC++;
				printf("\t Child %2d:%5d releasing resource at %.0lu:%lu\n", pname, getpid(), *clockPtr/(unsigned long)1e9, *clockPtr);
				writeRequest("output.txt", pname, *clockPtr, "RELEASING");
				sem_post(semaphore);
			}
			sem_wait(semaphore);
			previousTime = *clockPtr;
			sem_post(semaphore);
		}
		/* CHANCE PROCESS TERMINATES ITSELF (SUCCESS) */
		sem_wait(semaphore);
		diceRoll = getRandomNumber(0,100);
		if((*clockPtr >= (unsigned long)bound*((unsigned long)1e9) && diceRoll <= 5)){	
			if(requestedReso == true){ 
				printf("\t Child %2d:%5d releasing resource at %.0lu:%lu\n", pname, getpid(), *clockPtr/(unsigned long)1e9, *clockPtr);
				writeRequest("output.txt", pname, *clockPtr, "RELEASING");
			}
			printf("\t\t\t\t  -- Child %2d:%5d completes ----------------------\n", pname, getpid());
			writeStringSuccess("output.txt", pname, *clockPtr, "");
			resoPtr[pname-1] = 0;
			sem_post(semaphore);
			break;
		}
		sem_post(semaphore);
		/* HANDLING HUNG LOOP - SAFEGUARD TIMEOUT KILLS PROCESS IF STUCK */
		stop = time(NULL);
		if(stop - start > 5){
			if(requestedReso == true){ 
				printf("\t Child %2d:%5d releasing resource at %.0lu:%lu\n", pname, getpid(), *clockPtr/(unsigned long)1e9, *clockPtr);
				writeRequest("output.txt", pname, *clockPtr, "RELEASING");
			}
			printf("\t\t\t\t  -- Child %2d:%5d completes ----------------------\n", pname, getpid());
			writeOutDeadlock("output.txt", pname, *clockPtr);
			resoPtr[pname-1] = 0;
			sem_post(semaphore);
			break;
		}
	}
	sem_post(semaphore);
	if(r_wait(NULL) == -1){
		return 1;}
	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");}
	/* DETACH FROM SEGMENT */
	if(shmdt(rptr) == -1){
		perror("DETACHING SHARED MEMORY: shmdt()");
		return 1;}	
	shm_unlink("CLOCK");
	shm_unlink("RESC");
	shm_unlink("PIDS");
	exit(0);
}
void writeRequest(char * name, int child, unsigned long time, char * string){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "\t\t\t\tUSER: Child%3d:%6d\t%s at %.0lu:%lu\n", child, getpid(), string, time/(unsigned long)1e9, time);
	fprintf(fp, wroteline);
	fclose(fp);
	return;
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

void writeStringSuccess(char * name, int child, unsigned long time, char * string){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "\t  ---> USER Child %d:%d completed successfully.\t\tChild %d:%d terminated at %.0lu:%lu\n", child, getpid(), child, getpid(), time/(unsigned long)1e9, time);
	fprintf(fp, wroteline);
	fclose(fp);
	return;
}


void writeOutDeadlock(char * name, int child, unsigned long time){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "  ---> OSS instructed Child %d:%d to terminate at %.0lu:%lu\tChild %d:%d terminated at %.0lu:%lu\n", child, getpid(), time/(unsigned long)1e9, time, child, getpid(), time/(unsigned long)1e0, time);
	fprintf(fp, wroteline);
	fclose(fp);
	return;
}

void writeOut(char * name, int child, unsigned long time){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "\tChild %d:%d terminated at %.0lu:%lu\n", child, getpid(), time/(unsigned long)1e9, time);
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
