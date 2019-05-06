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

IPC MEM SEGMENT
	
	> Stores a struct containing the available resources, requests sent to OSS, and an (approve|deny) signal.  

PRINT MEM SEMGENT FORMAT

	> At various times the resource table is printed for viewing. The format of the table follows:

	Resources Row:  this row shows all available resources in the system.  Each column is a different resource.  Each resource type starts with 3 available.  So number of columns
	multiplied by 3 equals all resources available in the system.

	Requests Row: this row shows the requests being made by children for a specific resource.  The value of each column is the resource they are requesting, each request is for
	one resource of this resource type.  The column index of this row is the `process name` of the child process that is requesting the resource.  Defaults to -1 on init, will update
	to resource name when sending requests.

	Approved Row: this row is an array of signals that the OSS switchs on|off based on whether they approve or deny a childs request.  The value for each column is:
	-1 = inital val (if -1 at simulation end, that means the child never made a requests therefore parent never approved|denied a request
	0 = deny .. the parent denied the requests, the child reads the 0 and waits until it no longer is zero (while hung theres a chance the child will be terminated by deadlock algorithm
	1 = approved .. the parent approved the request, allowing the child to take the resource

OUTPUT

	and output file exists to log results, results are also shown to stdout.  Results include the number of deadlock prevention events where oss.c killed a child, 
	as well as the number of total children executed, and amount of resources that had been available in that simulation.

CHILD TERMINATION CRITERA

> DEADLOCK KILL
	the OS signals the child to terminate if it is caught in deadlock prevention, this is done if parent discovers a child requesting a resource that is already being requested (hence waited upon
	by another child)
> SUCCESSFUL TERMINATION
	there is a less than 10% chance a child will terminate successfully while running. If it has no resources held it will terminate.  If it has resources held, it will release them and close
	This is considered a successful child
> TIMEOUT TERMINATE
	if a child waits for more than 5 realtime seconds, it is killed

