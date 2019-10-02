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
int main_can_exit;
/* Data structures for rope mappings */

//Struct for rope mappings
//should this be static
typedef struct ropeMappings{
	int stake;
	bool severed;
} ropeMappings;

//struct ropeMappings ropeMappingArray[NROPES-1];

static ropeMappings ropeMappingsArray[NROPES-1];
/*
ropeMappings ropeMappingsArray[]={
    { .severed = false },
    { .severed = false },
};*/

int arr[] = { 1, 2, 3, 4 }; //test delete after	
/* Synchronization primitives */

static struct lock *lock_ropes_left;
static struct lock *lock_access_ropes;

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */


static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
        (void)arg;

        kprintf("Dandelion thread starting\n");
        
        
        while (1) {
        int randomRope = random() % NROPES;
                while (1) {
                        lock_acquire(lock_access_ropes);
                                if (lock_do_i_hold(lock_access_ropes)) {
                                                        if (ropeMappingsArray[randomRope].severed == false) {
                                                                ropeMappingsArray[randomRope].severed = true;
                                                                kprintf("Dandelion severed rope %d \n", randomRope);
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
                                                        }


                                }
                        lock_release(lock_access_ropes);
                        break;
                }
                if(ropes_left < 1){
                        break;
                }

        }
        kprintf("Marigold thread done\n");
        thread_yield();//Called when severed rope       
        thread_exit();
	
}



static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	//Marigold affects the stakes	

	kprintf("Marigold thread starting\n");

	
	while (1) {
  	int randomStake = random() % NROPES;
  		while (1) {
    			lock_acquire(lock_access_ropes);
    				if (lock_do_i_hold(lock_access_ropes)) {
      					for (int i = 0; i < NROPES; i++) {
        					if (ropeMappingsArray[i].stake == randomStake) {
          						if (ropeMappingsArray[i].severed == false) {
            							ropeMappingsArray[i].severed = true;
            							kprintf("Marigold severed rope %d from stake %d \n", i, ropeMappingsArray[i].stake);
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
          						}

        					}

      					}
    				}
			lock_release(lock_access_ropes);
			break;
  		}
		if(ropes_left < 1){
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
	
	int randomStake1;
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
						while(randomStake1 == randomStake2){ //So that the 2 stakes are not the same
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

	while(1){
		lock_acquire(lock_ropes_left);
		if(lock_do_i_hold(lock_ropes_left) && ropes_left == 0){
			lock_release(lock_ropes_left);
			break;
		} else {
			lock_release(lock_ropes_left);
		}
		
	}

	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	main_can_exit = 1;
	thread_yield();
	thread_exit();
}




//Make sure to deallocate all memory
int
airballoon(int nargs, char **args)
{
	int err = 0, i;
	
	main_can_exit = 0;
	(void)nargs;
	(void)args;
	(void)ropes_left;

	lock_ropes_left = lock_create("lockRopesCount"); //Don't forgt to deallocate this memory
	lock_access_ropes = lock_create("lockAccessRope");//Deallocate this too
	


	
	for (i = 0; i < NROPES; i++){

		ropeMappings temp = {.stake = i,.severed = false};
		ropeMappingsArray[i] = temp; 
		//For some reason I can't assign that array to {.hook = i,.stake = i,.severed = false};
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


	while(main_can_exit == 0){
		continue;
	}
	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
	lock_destroy(lock_ropes_left);
	lock_destroy(lock_access_ropes);
	kprintf("Main thread done\n");
	return 0;
}
