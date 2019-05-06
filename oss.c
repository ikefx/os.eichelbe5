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

#define SHM_KEY 0x9893
#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGSMEM ( PROT_EXEC | PROT_READ | PROT_WRITE )
#define max 50	// number of processes before succesful completion

struct rTable {
	int request[max];
	int approved[max];
	int resource[max];
	int requestC;
	int releaseC;
};

int findDupUtility(int * arr, int n);
void clearOldOutput();
void writeTable(char * name, int * resource, int * request, int * approved, int rSize);
void writeString(char * name, unsigned long time, char * string, int resourceType, int childName);
void writeOut(char * name,  unsigned long time, char * string);
int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);
void sigintHandler(int sig_num);
void printTable(int * resource, int * request, int * approved, int rSize);

char* sema = "SEMA5";
size_t clockSize = sizeof(unsigned long) + 1;
size_t resoSize  = sizeof(int) * max;
unsigned long * clockPtr = NULL;
int * resoPtr = NULL;
int * pidPtr  = NULL;

const size_t SHMSZ = sizeof(struct rTable);
struct rTable * rptr;
int shm1;

int main(int argc, char * argv[]){
	printf("\t\t--> OSS START <--\n\t\tParent PID: %d\n\n",getpid());
	clearOldOutput();
	srand(getpid());

	/* INIT VARIABLES */
	int resourceMax = 20 + getRandomNumber(-4,4);
	int procC = 0;
	int childPid;
	int procTotal = 0;
	int totalDeadlockKills = 0;
	pid_t pid;

	/* SEMAPHORE */
	sem_t * semaphore;
	if(getnamed(sema, &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}

	/* SET MEMORY SEGMENT AND INIT VARIABLES*/
	if((shm1 = shmget(SHM_KEY, SHMSZ, IPC_CREAT | 0666)) < 0){
		perror("Shared memory create: shmget()");
		exit(1);}
	if((rptr = shmat(shm1, NULL, 0)) == (void*) -1){
		perror("Shared memory attach: shmat()");
		exit(1);}
	for(int i = 0; i < max; i++){
		rptr->request[i] = -1;
		rptr->approved[i] = -1;}
	for(int i = 0; i < resourceMax; i++){
		rptr->resource[i] = 3;}
	rptr->requestC = 0;
	rptr->releaseC = 0;
	/* TIMER IN SHARED MEMORY */
	int fd_shm0 = shm_open("CLOCK", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm0, clockSize );
	clockPtr = (unsigned long*)mmap(0, clockSize, FLAGSMEM, MAP_SHARED, fd_shm0, 0);
	* clockPtr = 0;

	/* RESOURCE REQUEST IN SHARED MEMORY */
	int fd_shm1 = shm_open("RESC", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm1, resoSize);
	resoPtr = (int*)mmap(0, resoSize, FLAGSMEM, MAP_SHARED, fd_shm1, 0);
	for(int i = 0; i < 20; i++){
		resoPtr[i] = 0;
	}

	/* KILL PID LIST IN SHARED MEMORY */
	int fd_shm3 = shm_open("PIDS", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm3, resoSize );
	pidPtr = (int*)mmap(0, resoSize, FLAGSMEM, MAP_SHARED, fd_shm3, 0);

	/* WHILE TOTAL PROCESSES < 30 OR CHILDREN INCOMPELETE 	*
 	*  INCREASE CLOCK AND POSSIBLY CREATE ANOTHER CHILD 	*/ 	
	while(1){
		sem_wait(semaphore);
		/* WRITE TABLE ON OCCASION */
		if(*clockPtr % (unsigned long)50e9 == 0) writeTable("output.txt", rptr->resource, rptr->request, rptr->approved, resourceMax);	
		sem_post(semaphore);

		/* CREATE NEW PROCESS RANDOM CHANCE */
		if(procTotal < max && *clockPtr >= 1e9){	
			if(getRandomNumber(0,100) <= 20){
				procC++;
				procTotal++;	
				while(procC >= 18){
					signal(SIGINT, sigintHandler);
					*clockPtr += 5e8;
					wait(NULL);
					procC--;
				}
				char childName[128];
				char maxResour[128];
				sprintf(childName, "%d", procTotal);
				sprintf(maxResour, "%d", resourceMax);
				if((childPid = fork()) == 0){
					char * args[] = {"./user", childName, maxResour, '\0'};
					execvp("./user", args);
				}
			}
		}

		sem_wait(semaphore);
		* clockPtr += 5e8;

		/* WILL INTERCEDE IN CHILD WHILE LOOP : EVERY SECOND */
		if((*clockPtr % ((unsigned long)1e9)) == 0){
			int dupI = findDupUtility(resoPtr, max);
			if(dupI != -1){
				/* FOUND A DUPLICATE */
				resoPtr[dupI] = 0;
				pidPtr[dupI]  = 0;
				totalDeadlockKills++;
				procC--;
				sem_post(semaphore);
			}
			sem_post(semaphore);
		}
		sem_post(semaphore);

		/* APPROVE OR DENY RESOURCE REQUEST */
		sem_wait(semaphore);
		for(int i = 0; i < max; i++){
			if(rptr->request[i] > -1){
				int pname = i + 1;
				int rReq = rptr->request[i];
				if(rptr->resource[rReq] < 1 ){
				/* DENY */
					rptr->approved[pname-1] = 0;
				//	writeString("output.txt", *clockPtr, "\tOSS: OSS denied the request for R: ", rReq, pname );
				//	commenting out deny due to amount of lines it generates in output.txt
				}else{
				/* APPROVE */
					rptr->approved[pname-1] = 1;
					rptr->resource[rReq] -= 1;
					writeString("output.txt", *clockPtr, "\tOSS: OSS approved the request for R: ", rReq, pname );
				}
			}
		}
		sem_post(semaphore);

		/* BREAK OUT OF WHILE LOOP */
		if(*clockPtr > 210e9 && procTotal >= max) break;
	}
	time_t start = time(NULL);
	time_t stop;
	/* WAIT FOR INCOMPLETE PROCESS */
	while((pid = wait(NULL)) > 0){
		signal(SIGINT, sigintHandler);
		*clockPtr += 5e8;
		sem_wait(semaphore);
		if(procTotal > 0 && *clockPtr % ((unsigned long)1e9) == 0){
			int dupI = findDupUtility(resoPtr, max);
			if(dupI != -1){
				/* FOUND A DUPLICATE */
				resoPtr[dupI] = 0;
				pidPtr[dupI]  = 0;
				totalDeadlockKills++;
				procC--;
			}
		}
		sem_post(semaphore);
		/* COMMIT SUICIDE IF HUNG : TIMEOUT HANDLING */
		stop = time(NULL);
		if(stop - start > 20){
			printf("OSS: Timeout occured, shutting down.\n");			
			printf("\nOSS: %d Processes total were made.\n", procTotal);
			printf("\t\t--> OSS Terminated at %f <--\n", * clockPtr / 1e9);
			sem_close(semaphore);
			sem_unlink(sema);
			sem_destroy(semaphore);
			if(shmdt(rptr) == -1){
				perror("Shared memory detach: shmdt()");
				exit(1);}
			if(shmctl(shm1, IPC_RMID, 0) == -1){
				perror("Shared memory remove: shmctl()");
				exit(1);}
			shm_unlink("CLOCK");
			shm_unlink("RESC");
			shm_unlink("PIDS");
			exit(0);
		}
	}
	sem_wait(semaphore);	
	
	writeTable("output.txt", rptr->resource, rptr->request, rptr->approved, resourceMax);	

	printf("\nOSS: %d Processes total were made. %d were killed due to deadlock prevention\n", procTotal, totalDeadlockKills);
	printf("\tThere were %d different types of resources made available to children this run\n", resourceMax);
	printf("\t\t--> OSS Terminated at %f <--\n", * clockPtr / 1e9);
	

	FILE *fp;
	fp = fopen("output.txt", "a");
	char wroteline[355];
	sprintf(wroteline, "\nOSS Terminated at %f\nOSS: %d Processes total were made. %d were killed by deadlock prevention\nThere were %d differnt resource types, and %d requests and %d releases made.\n", 
		*clockPtr / 1e9, procTotal, totalDeadlockKills, resourceMax, rptr->requestC, rptr->releaseC);
	fprintf(fp, wroteline);
	fclose(fp);

	printTable(rptr->resource, rptr->request, rptr->approved, resourceMax);

	sem_post(semaphore);
	sem_close(semaphore);
	sem_unlink(sema);
	sem_destroy(semaphore);

	if(shmdt(rptr) == -1){
		perror("Shared memory detach: shmdt()");
		exit(1);}		
	if(shmctl(shm1, IPC_RMID, 0) == -1){
		perror("Shared memory remove: shmctl()");
		exit(1);}
	shm_unlink("CLOCK");
	shm_unlink("RESC");
	shm_unlink("PIDS");
	return 0;
}

int findDupUtility(int * arr, int n){
	/* FIND DUPLICATE IN ARRAY, RETURN INDEX OF SECOND INSTANCE */
	int idx, i, j;
	idx = -1;
	for( i = 0; i < n; i++){
		for( j = i+1; j < n; j++){
			if(arr[i] == arr[j] && arr[i] != 0){
				idx = j;
				break;
			}
		}
		if(idx != -1) break;
	}
	if(idx != -1){
		return idx;
	} else return -1;
}

void writeString(char * name, unsigned long time, char * string, int resourceType, int childName){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "%s%d by Child %d at %.0lu:%lu\n", string, resourceType, childName, (time / (unsigned long) 1e9), time);
	fprintf(fp, wroteline);
	fclose(fp);
	return;
}

void writeOut(char * name, unsigned long time, char * string){
	FILE *fp;
	fp = fopen(name, "a");
	char wroteline[355];
	sprintf(wroteline, "Deadlock examine at %.0lu:%lu %s\n", (time / (unsigned long) 1e9), time, string);
	fprintf(fp, wroteline);
	fclose(fp);
	return;
}

void writeTable(char * name, int * resource, int * request, int * approved, int rSize){
	FILE *fp;
	fp = fopen(name, "a");
	char temp[64];
	char wroteline[512] = "";
	strcat(wroteline, "\n\tTABLE FROM PARENT\n");

	strcat(wroteline, "\t-------- ---------- RESOURCE TABLE ---------- --------\n\tResources:\t");
	for(int i = 0; i < rSize; i++){
		sprintf(temp, "%2d ", resource[i]);
		strcat(wroteline, temp);
	}
	strcat(wroteline, "\n\tRequests:\t");
	for(int i = 0; i < max; i++){
		sprintf(temp, "%2d ", request[i]);
		strcat(wroteline, temp);
	}
	strcat(wroteline, "\n\tApproved:\t");
	for(int i = 0; i < max; i++){
		sprintf(temp, "%2d ", approved[i]);
		strcat(wroteline, temp);
	}
	strcat(wroteline, "\n\t-------- ---------- -------------- ---------- --------\n");
	fprintf(fp, wroteline);
	fclose(fp);
	return;
}

void clearOldOutput(){
	/* DELETE PREVIOUS LOG.TXT FILE */
	int status1 = remove("output.txt");
	if(status1 == 0){
		printf("--> Previous %s deleted.\n\n", "output.txt");
	}
	return;
}

void printTable(int * resource, int * request, int * approved, int rSize){
	printf("\n\tTABLE FROM PARENT\n");
	printf("\t-------- ---------- RESOURCE TABLE ---------- --------\n\t");
	printf("Resources:\t");
	for(int i = 0; i < rSize; i++){
		printf("%2d ", resource[i]);
	}
	printf("\n\t");
	printf("Requests:\t");
	for(int i = 0; i < max; i++){
		printf("%2d ", request[i]);
	}
	printf("\n\tApproved:\t");
	for(int i = 0; i < max; i++){
		printf("%2d ", approved[i]);
	}
	printf("\n\t-------- ---------- -------------- ---------- --------\n");
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
	shm_unlink("PIDS");
	shm_unlink("RESC");
	sem_unlink(sema);
	kill(0,SIGTERM);
	exit(0);
}
