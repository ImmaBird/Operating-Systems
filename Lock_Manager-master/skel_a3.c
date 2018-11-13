/**********************************************
 *                                            *
 *   IPC One parent process NPROC children    *
 *                                            *
 * To compile:				      *
 *   gcc -g -o skel_a3 skel_a3.c              *
 *                                            *
 **********************************************/

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>




#define DONTPRINT
#define NPROC	       (16)  /* number of children processes */
#define TRUE		1
#define FALSE		0
#define MARKED 		1   /* lock reserved (lock is taken by a child) */
#define NOTMARKED 	0   /* lock available (no child process owns this lock) */
#define MAXCHILDLOCKS	10   /* max resource a child can hold before requesting
			       'release locks' from the LockManager */


#define NO_DEADLOCK 		0	/* there is no deadlock */
#define DEADLOCK_DETECTED 	1	/* Deadlock detected    */

#define MAXLOCKS		10	/* Total available resources (size of the lock table) */





/*
 * Children send this message to request a lock.
 */
#define LOCK		100	/* Child requests to lock a resource */
#define RELEASE		200	/* Child requests to release all its resources */
struct msg_requestLock {
	int lockID;	/* this a number from 0 up to (MAXLOCKS-1) */
	int Action;	/* LOCK, RELEASE */
};




/*
 * LockManager sends status of request to children
 */
#define GRANTED 	0
#define NOT_GRANTED	1
#define YOU_OWN_IT	2
#define PREVENT		3
struct msg_LockStatus {
	int status;
	int by_child;		/* if not granded, who owns it */
};






/*
 * Structure the LockManager holds (this is a single lock)
 */
struct lock {
	int marked;
	int by_child;
};


/*
 * 'lock' holds all the resources
 */
struct lock locks[MAXLOCKS];   /* MAXLOCKS locks for the manager */





int deadlock = NO_DEADLOCK;	/* When deadlock occurs, exit     */
int pid [NPROC];               	/* Process ids                    */

/*
 * Called at the end to cleanup
 */
void finish() {
	int i;
	for(i = 0; i < NPROC; i++)
		kill( pid[i], 9);
	exit(0);
}




/*
 * Code for the child processes
 */
void child (int pid, int req, int ack) {
	int rand_lock;		/* a random lock request        */
	int count = 0;		/* It is used for num of locks  */

	struct msg_requestLock MSG;	/* message from child (me) to parent */
	struct msg_LockStatus  STAT; 	/* message from parent to child (me) */

	struct timeval tt;

	(void) gettimeofday(&tt, NULL);
	srand(tt.tv_sec * 1000000 + tt.tv_usec);

	for(;;) {
		MSG.lockID  = rand() % MAXLOCKS;
		MSG.Action  = LOCK;
#ifdef DONTPRINT
		printf("\tChild %d: Requesting lock %d . . .\n", pid, MSG.lockID);
		fflush(stdout);
		#endif

		/*
	 	 * Both calls are blocked if there is nothing to do.
	 	 */

		write( req, (char *) &MSG,  sizeof(MSG));
		read ( ack, (char *) &STAT, sizeof(STAT));

		if( STAT.status == GRANTED ) { 	    /* Success got lock */
			count++;
			#ifdef DONTPRINT
			printf("\tChild %d: Got lock %d (%d).\n", pid, MSG.lockID, count);
			fflush(stdout);
			#endif
		}

#ifdef TRACE
		if( STAT.status == GRANTED )
			printf("\tChild %d: Got lock.\n", pid);
		else if( STAT.status == NOT_GRANTED)
			printf("\tChild %d: Child %d owns this lock.\n", pid, STAT.by_child);
		else if( STAT.status == YOU_OWN_IT)
			printf("\tChild %d: I own this lock.\n", pid);

		printf("\tChild %d: Owns %d locks now.\n", pid, count);
		fflush(stdout);
#endif

		if( STAT.status == NOT_GRANTED ) {
			#ifdef DONTPRINT
			printf("\tChild %d waiting for lock %d\n", pid, MSG.lockID);
			fflush(stdout);
			#endif

			/*
			 * I will get it shortly or the LockManager
		  	 * will NOT give it to me to prevent a deadlock.
			 */
			read ( ack, (char *) &STAT, sizeof(STAT));
			if( STAT.status == GRANTED ) {
				count++;
				#ifdef DONTPRINT
				printf("\tChild %d: Got lock %d (%d).\n", pid, MSG.lockID, count);
				#endif
			}
			else if( STAT.status == PREVENT ) {
				#ifdef DONTPRINT
				printf("CHILD: %d Will try again, (preventing)\n", pid);
				fflush(stdout);
				#endif
			}
			else {
				#ifdef DONTPRINT
				printf("CHILD: %d    FATAL ERROR\n", pid);
				fflush(stdout);
				#endif
				exit(-1);
			}
		}

		if( count >= MAXCHILDLOCKS ) {
			/*
			 * Child sends request to release all its locks
			 */
			 #ifdef DONTPRINT
			printf("\tChild %d: Requesting RELEASE locks.\n", pid);
			fflush(stdout);
			#endif

			MSG.Action=RELEASE;
			write(req,(char *) &MSG,sizeof(struct msg_requestLock));

			count = 0;

			sleep(1);
		}

	} /* for(;;) */
} /* child */


// My variables ********************************************************************
int last_resource_given = -1;
int waiting[MAXLOCKS][NPROC];
int new_wait = -1;
int wait_for = -1;
int graph[NPROC];
//**********************************************************************************


void print_lock_table(){
	printf("\n");
	for(int i = 0; i < MAXLOCKS; i=i+1){
		if(locks[i].marked == MARKED){
			printf("Lock %d : %d, Waiting : ", i, locks[i].by_child);
		 for(int j = 0; j < NPROC; j=j+1){
			if(waiting[i][j] != -1){
				if(j == 0){
					printf("%d", waiting[i][j]);
				}else{
					printf(", %d", waiting[i][j]);
				}
			}
		}
		printf("\n");
		fflush(stdout);
		}
	}
	printf("\n");
	fflush(stdout);
}

int CheckForDeadLock() {
	int next_child = graph[new_wait];
	while(1){
		if(graph[next_child] == -1){
			return NO_DEADLOCK;
		}else if(next_child == new_wait){
			return DEADLOCK_DETECTED;
		}else{
			next_child = graph[next_child];
		}
	}
}



/*******************************************************
 *                                                     *
 * LockManager():                                      *
 *               Task to determine deadlock            *
 *               Also to release and give locks        *
 *		 Terminates when LiveLock is detected  *
 *******************************************************/

int LockManager( int q, struct msg_requestLock ChildRequest, int respond[NPROC] ) {
	int i;
	struct msg_LockStatus  STAT;
	int deadlock=NO_DEADLOCK;

	if( ChildRequest.Action == RELEASE ) {
		/*
		 *      ***TODO*** (1)
		 * Release child's resources.
		 * Give resources to children that might be waiting
		 * for these resources.
		 */

		 for(int i = 0; i < MAXLOCKS; i=i+1) {
			 if(locks[i].marked == MARKED && locks[i].by_child == q) {
				 if(waiting[i][0] != -1){
					 locks[i].by_child = waiting[i][0];

					 for(int j = 0; j < NPROC; j=j+1){
						 if(graph[j] == q && j != waiting[i][0]){
							 graph[j] = waiting[i][0];
						 }
					 	}
						graph[waiting[i][0]] = -1;



					 STAT.status=GRANTED;
					 write(respond[waiting[i][0]], (char *) &STAT, sizeof(STAT));

					 for(int j = 1; j < NPROC; j=j+1){

							 waiting[i][j-1] = waiting[i][j];
							 waiting[i][j] = -1;
					 }


				 }else{
					 locks[i].marked = NOTMARKED;
				 }
			 }
		 }
	}


	if( ChildRequest.Action == LOCK ) {
		int t_lock;
		t_lock = ChildRequest.lockID;
		if( locks[t_lock].marked == NOTMARKED ) {
			/*
		 	 *      ***TODO*** (2)
		 	 * Give requested lock to child
		 	 */

			 locks[t_lock].marked = MARKED;
			 locks[t_lock].by_child = q;

			 STAT.status = GRANTED;
			 write(respond[q], (char *) &STAT, sizeof(STAT));

		}
		else { /* lock is not free */
			if( locks[t_lock].by_child == q ) {
				/*
				 * Tell child that this lock is already owned
			 	 * by this (the requestor) child
				 */

				STAT.status=YOU_OWN_IT;
				write(respond[q], (char *) &STAT, sizeof(STAT));

			}
			else { /* lock taken by another child */
				/*
		 	 	 *      ***TODO*** (3)
				 * Lock is owned by another child, need to wait!
				 * Set up the waiting list.
				 */

				 for(int i = 0; i < NPROC; i=i+1){
					 if(waiting[t_lock][i] == -1){
						 waiting[t_lock][i] = q;
						 break;
					 }
				 }

				 new_wait = q;
				 wait_for = t_lock;
				 graph[q] = locks[t_lock].by_child;

				/*
				 * Now tell the child that the Lock will
				 * not be given (because it's owned by
				 * someone else.
				 */

				STAT.status=NOT_GRANTED;
				STAT.by_child = locks[t_lock].by_child;

				write(respond[q], (char *) &STAT, sizeof(STAT));

				/*
		 	 	 *      ***TODO*** (4)
				 * Print the lock table and the waiting list
				 * so that YOU see what your program is doing
				 */



				print_lock_table();



				/*
		 	 	 *      ***TODO*** (5)
				 * Implement the 'CheckForDeadLock()'
				 * function.  If you see a deadlock return
				 * DEADLOCK_DETECTED as shown below.
				 */

				if( CheckForDeadLock() == DEADLOCK_DETECTED ) {


					printf("NOW ROLLBACK\n");
					/*
		 	 	 	 *      ***TODO*** (6)
				 	 * Detected Deadlock! you need to
					 * prevent it.  Rollback so that
	`				 * you will not give the lock to
					 * the requestor.
					 */

					 print_lock_table();
					 if(new_wait != -1 && wait_for != -1){
					 for(int i = 0; i < NPROC; i=i+1){
						 if(waiting[wait_for][i] == new_wait){
							 waiting[wait_for][i] = -1;
						 }
					 }
					 graph[new_wait] = -1;
				 }



					/*
					 * OK we rolledback, now notify the
					 * child that the lock will not be
					 * given to it in order to prevent
					 * the deadlock.
					 */
					STAT.status=PREVENT;
					write(respond[q], (char *) &STAT, sizeof(STAT));


					/*
		 	 	 	 *      ***TODO*** (7)
					 * Now that we prevented the deadlock
					 * check to see if there are any free
					 * locks.  If not, we will be
					 * preventing deadlocks for ever.
					 * If there are no more free
					 * locks/resources then we got a
					 * LiveLock and we should
					 * 1. Print a message on the screen
					 * 2. Print the lock table and the
					 *    waiting list/graph.
					 * 3. Terminate.
					 */

					 int ll = 1;
					 for(int i = 0; i < MAXLOCKS; i=i+1){
						 if(locks[i].marked == NOTMARKED){
							 ll = 0;
						 }
					 }

					 if(ll){
						 printf("LIVELOCK:\n");
						 print_lock_table();
						 finish();
					 }
				}

			}

		}
	}


	//
	// if ChildRequest.Action is neither RELEASE nor LOCK, you got a protocol error.
	//



	return(deadlock);
}





/******************************************************************
 *
 ******************************************************************/
int main(int argc, char** argv) {
	int i;
	int listen[NPROC];
	int respond[NPROC];
	int ii;

	// Initialize Waiting
	for(int i = 0; i < MAXLOCKS; i=i+1){
		for(int j = 0; j < NPROC; j=j+1){
			waiting[i][j] = -1;
		}
	}
	for(int j = 0; j < NPROC; j=j+1){
		graph[j] = -1;
	}

	struct msg_requestLock ChildRequest;


	/*
	 * Arrange for termination to call the cleanup procedure
	 */
	signal( SIGTERM, finish );


	/*
	 * initialize, don't carre about child
	 */
	for( ii = 0; ii < MAXLOCKS; ii++) {
		locks[ii].marked = NOTMARKED;
	}


	/*
	 * Initialize pipes and fork the children processes
	 */
	for( i = 0; i < NPROC; i++ ) {
		int parent_to_child[2];
		int child_to_parent[2];

		/*
	  	 * Create the child -> parent pipe.
		 */
    		pipe(child_to_parent);
    		listen[i] = child_to_parent[0];
    		fcntl (listen[i], F_SETFL, O_NONBLOCK);

    		/*
		 * Create the parent -> child pipe.
		 */
    		pipe(parent_to_child);
    		respond[i] = parent_to_child[1];
    		fcntl (respond[i], F_SETFL, O_NONBLOCK);

    		/*
		 * Create child process.
		 */

		if ((pid[i] = fork()) == 0) {
  			/*
	 		 * *********** Child process code. ***********
	 		 */
			signal (SIGTERM, SIG_DFL);
			close (parent_to_child[1]);
			close (child_to_parent[0]);

			child (i, child_to_parent[1], parent_to_child[0]);

			_exit(0);
    		}

  		close (child_to_parent[1]);
  		close (parent_to_child[0]);
  	}




	/*
	 * For this assignment there will never be a deadlock because our
	 * LockManager prevents deadlocks
	 */
	while( deadlock != DEADLOCK_DETECTED ) {
		int q;
		for( q = 0; q < NPROC; q++) {
			unsigned char buffer[2];
			int nb = read(listen[q], (char *) &ChildRequest,
				      sizeof(ChildRequest));
			if(nb == sizeof(ChildRequest) ) {
				deadlock = LockManager(q, ChildRequest, respond);
			}
		}
	}

	/*
	 * Just be nice if your LockManager detects but does not
         * prevent a deadlocks, kill all children processes.
	 */
	finish();
}
