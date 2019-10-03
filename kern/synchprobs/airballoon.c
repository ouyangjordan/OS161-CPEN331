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
	int stake;
	bool severed;
} ropeMappings;

//Array of rope mappings 
//Index of array is the rope (i.e hooks for dandelion and the stake in the rope index is the stake position)
static ropeMappings ropeMappingsArray[NROPES-1];

/* Synchronization primitives */

static struct lock *lock_ropes_left; //Lock to lock the integer containing how many ropes are left un-severed
static struct lock *lock_access_ropes; //Lock to access the ropes

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */

/*
Invariants: There are no invariants

Locking protocols: Must lock the data structure when modifying them. You must have the lock to decrement
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
        
        while (1) {
        	int randomRope = random() % NROPES;//Dandelion selects random rope to sever
        	while (1) {
                	lock_acquire(lock_access_ropes);
                        if (lock_do_i_hold(lock_access_ropes)) {
                        	if (ropeMappingsArray[randomRope].severed == false) {
                                	ropeMappingsArray[randomRope].severed = true;
                                        kprintf("Dandelion severed rope %d \n", randomRope);
					lock_release(lock_access_ropes); //Release the lock before yielding the thread
                                        thread_yield();
                                        while (1) { //This while loop is to gain access to the variable which holds
						//how many ropes are left, an exit condition
                                        	lock_acquire(lock_ropes_left);
                                                if (ropes_left > 0 && lock_do_i_hold(lock_ropes_left)) {//ensure that we hold 
												//the lock before modifying
                                                	ropes_left = ropes_left - 1;
                                                        lock_release(lock_ropes_left);
                                                        break;
                                                } else {
							continue;
                                                }
                                        }
                                 } else {
					lock_release(lock_access_ropes);
				 } 
                        }
                        break;
                }
                if(ropes_left < 1){
                        break;
                }

        }

        kprintf("Dandelion thread done\n");
        thread_yield();//Called when severed rope       
        thread_exit();
	
}



static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Marigold thread starting\n");

	while (1) {
  	int randomStake = random() % NROPES;
  	while (1) {
        	lock_acquire(lock_access_ropes);
    		if (lock_do_i_hold(lock_access_ropes)) {
      			for (int i = 0; i < NROPES; i++) { //Iterate through the array of rope and rope mappings to find the correct stake
							//and see that it is not severed
        		if (ropeMappingsArray[i].stake == randomStake) {
          			if (ropeMappingsArray[i].severed == false) {
            				ropeMappingsArray[i].severed = true;
            				kprintf("Marigold severed rope %d from stake %d \n", i, ropeMappingsArray[i].stake);
					lock_release(lock_access_ropes);			
					thread_yield();
            				while (1) {
              					lock_acquire(lock_ropes_left);
              					if (ropes_left > 0) {
                					ropes_left = ropes_left - 1;
                					lock_release(lock_ropes_left);
							break;
              					} else {
						lock_release(lock_ropes_left);
						}
            				}	
          			} else {
					lock_release(lock_access_ropes);
				}
        		}
			}
    		}
		break;
  		}
		if(ropes_left < 2){ //If there are left than 2 ropes to switch exit flower killer
			break;
		}
	
	}
	kprintf("Marigold thread done\n");	
	thread_yield();//Called when severed rope	
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
	
	while (1) {
		randomStake1 = random() % NROPES;
		while (1) {
			lock_acquire(lock_access_ropes);
			if (lock_do_i_hold(lock_access_ropes)){
				for(int i = 0; i < NROPES; i++){
					if(ropeMappingsArray[i].stake == randomStake1 && ropeMappingsArray[i].severed == false){
						randomStake1Rope = i;
						randomStake2 = random() % NROPES;
						while(randomStake1 == randomStake2){ //Ensure that the 2 stakes are not the same
							randomStake2 = random() % NROPES;
						}
						for(int i = 0; i < NROPES; i++){
							if(ropeMappingsArray[i].stake == randomStake2 && ropeMappingsArray[i].severed == false){
								randomStake2Rope = i;
								ropeMappingsArray[randomStake1Rope].stake = randomStake2;
								ropeMappingsArray[randomStake2Rope].stake = randomStake1;
								kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d \n", randomStake1Rope , randomStake1, randomStake2);
								kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d \n", randomStake2Rope , randomStake2, randomStake1);
								thread_yield();
								break;
							}
						}
						
					}
				}
			}
			lock_release(lock_access_ropes);
			break;
		}
		if(ropes_left == 0){
		break;
	}	

	}


	kprintf("Lord FlowerKiller thread done\n");

	//Lord flower killer cannot swap 2 ropes
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
			continue;
		}
	}

	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	main_can_exit = 1; //Signal that the main thread can exit in airballoon
	thread_yield();
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
	lock_access_ropes = lock_create("lockAccessRope");//Lock to access rope
	
	for (i = 0; i < NROPES; i++){ //Initialize our rope datastructure that contains the ropes and stake mappings (initially 1-1 i.e rope 4 is attached to stake 4)
		ropeMappingsArray[i] = (ropeMappings) {.stake = i, .severed = false};
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
		continue;
	}
	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
	//Deallocate memory used
	lock_destroy(lock_ropes_left);
	lock_destroy(lock_access_ropes);
	kprintf("Main thread done\n");
	ropes_left = 16; //This is so that the test can be run over again in sp1

	return 0;
}
