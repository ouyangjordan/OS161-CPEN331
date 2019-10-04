/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define N_LORD_FLOWERKILLER 8
#define NROPES 16

static int ropes_left = NROPES; 
int main_can_exit; //This variable indicates when the airballoon function is finished

/* Data structures for rope mappings */

//Struct for rope mappings
typedef struct ropeMappings{
	struct lock *rope_lock;
	volatile int stake;
	volatile bool severed;
} ropeMappings;

//Array of rope mappings 
//Index of array is the rope (i.e hooks for dandelion and the stake in the rope index is the stake position)
static ropeMappings ropeMappingsArray[NROPES-1];

/* Synchronization primitives */

static struct lock *lock_ropes_left; //Lock to lock the integer containing how many ropes are left un-severed
/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */


/*
Invariants: There are no invariants

Locking protocols: Must lock the data structure when modifying them. There is an array
of 16 rope mappings, each is a rope struct with its own lock.
You must hold the lock(s) to modify them.

You must have the lock to decrement
the ropes left count and you must have the lock to swap ropes or sever them

Exit conditions:

Threads know when they are done when there is no ropes left to sever, all checking the global variable
ropes_left

The airballoons function exits when the int variable main_can_exit is modified
when balloon function completes.

*/

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
        (void)arg;

        kprintf("Dandelion thread starting\n");
        
        while (ropes_left > 0) {
        	int randomRope = random() % NROPES;//Dandelion selects random rope to sever
                
		lock_acquire(ropeMappingsArray[randomRope].rope_lock);
		if(ropeMappingsArray[randomRope].severed == false){
                	
			ropeMappingsArray[randomRope].severed = true;
                        kprintf("Dandelion severed rope %d \n", randomRope);
			lock_release(ropeMappingsArray[randomRope].rope_lock); //Release the lock before yielding the thread
                        thread_yield();
                                       
			 //This is to gain access to the variable which holds
						//how many ropes are left, an exit condition
                        lock_acquire(lock_ropes_left);
                        if (ropes_left > 0) {
                        	ropes_left = ropes_left - 1;
			}
                        lock_release(lock_ropes_left);
        	} else {
			lock_release(ropeMappingsArray[randomRope].rope_lock);
		}
	}

        kprintf("Dandelion thread done\n");
        
	thread_exit();
	
}


static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Marigold thread starting\n");

	while (ropes_left > 0) {
  		int randomStake = random() % NROPES;
        
		for(int i = 0; i < NROPES; i++){ //In this loop we look for the stake that is generated from random
			if(ropeMappingsArray[i].stake == randomStake){
				lock_acquire(ropeMappingsArray[i].rope_lock);
				
				if(ropeMappingsArray[i].stake == randomStake && ropeMappingsArray[i].severed == false){
				//ensure that the stake position did not change due to flower killer and that
				//the rope is not severed yet

					ropeMappingsArray[i].severed = true;
					kprintf("Marigold severed rope %d from stake %d \n", i, ropeMappingsArray[i].stake);
					lock_release(ropeMappingsArray[i].rope_lock);
					thread_yield();
		
					lock_acquire(lock_ropes_left);
					if(ropes_left > 0){
						ropes_left = ropes_left - 1;
					}
					lock_release(lock_ropes_left);

				} else {
					lock_release(ropeMappingsArray[i].rope_lock);
				}
			}
		}    		
	
	}
	kprintf("Marigold thread done\n");	
	
	thread_exit();
}

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	
	int randomStake1; //Initialize the stakes flower killer plans to switch
	int randomStake1Rope;

	int randomStake2;
	int randomStake2Rope;

	kprintf("Lord FlowerKiller thread starting\n");
	
	while (ropes_left > 1) { //If there are less than 2 ropes exit because flower killer needs to swap 2 ropes
		randomStake1 = random() % NROPES;
		for(int i = 0; i < NROPES; i++){ //as with marigold, iterate through ropes (hooks index) to find corresponding stake
			if(ropeMappingsArray[i].stake == randomStake1){
				randomStake1Rope = i; //The index is the rope so save the index
				lock_acquire(ropeMappingsArray[randomStake1Rope].rope_lock);
				if(ropeMappingsArray[randomStake1Rope].stake == randomStake1 && ropeMappingsArray[randomStake1Rope].severed == false){
					
					//I did not want to iterate through for another stake as it raised
					//issues so I will just swap with a random rope

					randomStake2Rope = random() % NROPES;
					while(randomStake2Rope == randomStake1Rope){
						randomStake2Rope = random() % NROPES;
					} //ensure that we are not swapping the same stake
							
					lock_acquire(ropeMappingsArray[randomStake2Rope].rope_lock);
                			if(ropeMappingsArray[randomStake2Rope].severed == false){
                        			randomStake2 = ropeMappingsArray[randomStake2Rope].stake; //Unlock each lock as it is used as to not create deadlocks
						ropeMappingsArray[randomStake2Rope].stake = randomStake1;
						lock_release(ropeMappingsArray[randomStake2Rope].rope_lock);
                                                ropeMappingsArray[randomStake1Rope].stake = randomStake2;
						lock_release(ropeMappingsArray[randomStake1Rope].rope_lock);
                                                kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d \n", randomStake1Rope , randomStake1, randomStake2);
                                                kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d \n", randomStake2Rope , randomStake2, randomStake1);	
	
		
						thread_yield();
						continue;
                                       	} else {
						//release these locks
						lock_release(ropeMappingsArray[randomStake1Rope].rope_lock);
						lock_release(ropeMappingsArray[randomStake2Rope].rope_lock);
						thread_yield();
						continue;
					}
				} else {
					lock_release(ropeMappingsArray[randomStake1Rope].rope_lock);
					thread_yield();
					continue;
				}
			}
		}

	}


	kprintf("Lord FlowerKiller thread done\n");
	
	thread_exit();
}
static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");

	while(1){//This runs constantly in the background to see if the ropes are at 0 or not
		if(ropes_left < 1){	
			break;
		} else {
			thread_yield(); //Yield thread if not done yet
		}
	}

	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	main_can_exit = 1; //Signal that the main thread can exit in airballoon
	kprintf("Balloon thread done\n");
	thread_exit();
}




//Make sure to deallocate all memory
int
airballoon(int nargs, char **args)
{
	int err = 0, i;
				
	main_can_exit = 0;//When this is changed to 1 by balloon fucntion this thread can exit
	(void)nargs;
	(void)args;
	(void)ropes_left;

	lock_ropes_left = lock_create("lockRopesCount"); //Lock to decrement rope count when severing a rope
	for (i = 0; i < NROPES; i++){ //Initialize our rope datastructure that contains the ropes and stake mappings (initially 1-1 i.e rope 4 is attached to stake 4)
		ropeMappingsArray[i] = (ropeMappings) {.stake = i, .severed = false, .rope_lock = lock_create("")};
	}

	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;
	for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
		err = thread_fork("Lord FlowerKiller Thread",
				  NULL, flowerkiller, NULL, 0);
		if(err)
			goto panic;
	}
	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;


	while(main_can_exit == 0){ //This thread is waiting on the signal that it can exit from balloon()
		thread_yield();
	}
	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
	//Deallocate memory used
	lock_destroy(lock_ropes_left);
	kprintf("Main thread done\n");
	
	for (int i = 0; i < NROPES; i++){
		lock_destroy(ropeMappingsArray[i].rope_lock);
	} 
	return 0;
}
