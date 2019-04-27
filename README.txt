Neil Eichelberger Project 5 
Resource and Deadlock Prevention

oss.c 

	oss.c is the parent process.  It forks up to a maximum number of child processes (max is a global value, defaulting to 30).  
	oss.c is successful if all 30 processes are created and ran to termination successfuly.  Parent will terminate once all childs are complete.

	It creates children at random times, increasing a logical clock 500 milliseconds each attempt.  Only 18 children can run concurrently.

	Every whole second oss.c examines children resource request to see if any are requesting the same resource.  If so, oss.c will signal one of 
	those children to terminate.  Since the deadlock prevention algorith runs every whole second and children request every 500 milliseconds, there
	is ample time for children to respond to parent kill signal.
	
	
user.c

	is the child process that is forked from parent.  It runs for a random amount of time from between 1 and BOUND seconds.  BOUND is randomly determined
	with RNG, but child must run for at least one second.  During their lifetime children will randomly request resources from parent.  If a child has 
	already requested a resource, then they randomly release those resources.  

	Once the logical clock reaches the maxima of user.c's BOUND value, there's a chance the user.c will terminate (successfully).  

	Which resource the user.c process requests is random, but the resource types range from R0 to R(20 +- 20%).  Therefore it is possible that to user.c
	processes will be asking for the same resource at the same time.  

	Parent's deadlock protection algorithm may notice the duplicate resource requests from concurrent children and order 1 of them to terminate.

SHARED MEMORY ITEMS

	> logical Clock: (in nanoseconds, formatted to be seen as whole seconds and nanoseconds)
		the clock is incremented by 500 milliseconds each iteration, and is monitored by children and parent
	
	> request Table: An array of integers representing each child's active request.  Since resources vary in types R0 to R(20 +- 4) that means there
	are min of 16 and max of 24 different resource types each simulation.  Deadlocks scenarios are more likely if there are less resource types available.
	The indices of the array repesent the name of the child processes.  Each child is named 1 to max.  So request[0] is the request of Child1, request[1] is 
	the resource request of Child2, etc.

	user.c places their Resource request in this array, and oss.c examines it every second: looking for duplicates (race conditions)

	> pid table: each child process places their pid in this array, where indice represents the name of the child (again: pids[0] would hold the pid of child1)
	when race conditions are found by the deadlock detection algorithm, oss.c changes the pid of a culprit to 0.  This serves as a signal to that child to terminate.

	user.c monitors the pid array, and if OSS sets the child process's pid to 0, that serves as the signal that oss.c is instructing child to terminate due to risk
	of deadlock.

OUTPUT

	and output file exists to log results, results are also shown to stdout.  Results include the number of deadlock prevention events where oss.c killed a child, 
	as well as the number of total children executed, and amount of resources that had been available in that simulation.

TIMEOUT HANDLING

	The program has a risks of getting hung in its primary loop.  This is due to the BOUND value of children. Since the BOUND is random, it is possible that 18+ children processes
	will select a bound that the clock will not reach, this can cause the program to hang.  If children become hung in this manner, they will terminate after 15 real time seconds.
	Since no more than 18 proceses can be run at once, if hung the timeout will be required to proceed.  Please rerun the simulation if timeout occurs.
	
