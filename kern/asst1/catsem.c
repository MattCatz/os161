/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in
 * this file.
 */


/*
 *
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

/*
 *
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

#define TRUE 1
#define FALSE 0
#define MAXTIME 2
#define MAXEAT 10


static struct semaphore *done;
static struct semaphore *full;
static struct semaphore *mutex; // Kitchen
static struct semaphore *dish_mutex;
static struct semaphore *cats_queue;
static struct semaphore *mice_queue;
static volatile int all_dishes_available;
static volatile int cats_wait_count;
static volatile int mice_wait_count;
static volatile unsigned int cat_magic_number;
static volatile unsigned int mouse_magic_number;
static volatile int first_cat;
static volatile int first_mouse;
static volatile int mice_eating;
static volatile int cats_none_eating;
static volatile int dish_one_busy;
static volatile int dish_two_busy;
static volatile int done_eating;
static volatile int cats_eating;
static volatile int mice_eating;
static volatile int wait_to_finish;

/*
 *
 * Function Definitions
 *
 */
 
void hungry_cat(long unsigned id) {
  int mydish;

  kprintf("    Cat %ld got hungry\n",id);
  assert(mutex->count == 1 || mutex->count == 0);
  P(mutex); // cats get kitchen
  cats_wait_count++;
  V(mutex);  // each cat/mouse lets the next one in
             // The last one leaves the lock for the first cat/mouse
  
  P(cats_queue);
  cats_wait_count--;
  // at this point everyone but the first cat should be in the queue.
  if (first_cat == TRUE) {
    first_cat = FALSE;
    kprintf("    Cat %ld first in the cat queue.\n", id); /* cat name */
    // sync check that there is only one first cat
    assert(0xDEADBEEF == cat_magic_number++);
    P(dish_mutex); /*protect shared variables*/
    kprintf(">>> Cat %ld got into kitchen.\n", id); /* cat name */
    if (cats_wait_count > 0) {
      kprintf("... Letting a cat \n");
      V(cats_queue); // first cat brings the next cat in the queue
      wait_to_finish = TRUE;
    }
  } else {
    kprintf(">>> Cat %ld let in the kitchen.\n", id); /* cat name */
  }
 
  if (dish_one_busy == FALSE) {
    dish_one_busy = TRUE;
    mydish = 1;
  } else {
    assert(dish_two_busy == FALSE);
    dish_two_busy = TRUE;
    mydish = 2;
  }
 
  kprintf("*** Cat %ld eating bowl %d.\n", id, mydish); /* cat name */
  clocksleep(random() % MAXTIME); /* enjoys food */
  assert(mice_eating == 0);
 
  if (mydish == 1) {
    dish_one_busy = FALSE;
  } else {
    assert(mydish == 2);
    dish_two_busy = FALSE;
  }
 
  P(mutex);
  if(wait_to_finish == FALSE) {
    kprintf("*** Cat %ld finished eating last.\n", id); /* done. */
    // sync check to make sure there is only one last cat
    assert(0xDEADBEEF == --cat_magic_number);
    cat_magic_number = 0xDEADBEEF;
    done_eating = FALSE;
    first_cat = TRUE;
    V(cats_queue);
    V(dish_mutex); // the last cat lets the next two in
  } else {
    wait_to_finish = FALSE;
    kprintf("*** Cat %ld finished eating first.\n", id); /* done. */
  }
  V(mutex);
  kprintf("<<< Cat %ld left the kitchen.\n", id); /* cat name */
}

void hungry_mouse(unsigned long id) {
  int mydish;

  kprintf("    Mouse %ld got hungry\n",id);
  assert(mutex->count == 1 || mutex->count == 0);
  P(mutex); // cats get kitchen
  mice_wait_count++;
  V(mutex);  // each cat/mouse lets the next one in
             // The last one leaves the lock for the first cat/mouse

  P(mice_queue);
  mice_wait_count--;
  // at this point everyone but the first cat should be in the queue.
  if (first_mouse == TRUE) {
    first_mouse = FALSE;
    kprintf("    Mouse %ld first in mouse queue.\n", id); /* cat name */
    P(dish_mutex); /*protect shared variables*/
    kprintf(">>> Mouse %ld got into kitchen.\n", id); /* cat name */
    if (mice_wait_count > 0) {
      kprintf("... Letting in other mouse \n");
      V(mice_queue); // first cat brings the next cat in the queue
      wait_to_finish = TRUE;
    }
  } else {
    kprintf(">>> Mouse %ld let in the kitchen.\n", id); /* cat name */
  }

  if (dish_one_busy == FALSE) {
    dish_one_busy = TRUE;
    mydish = 1;
  } else {
    assert(dish_two_busy == FALSE);
    dish_two_busy = TRUE;
    mydish = 2;
  }

  mice_eating++;
  kprintf("*** Mouse %ld eating bowl %d.\n", id, mydish); /* cat name */
  clocksleep(random() % MAXTIME); /* enjoys food */

  if (mydish == 1) {
    dish_one_busy = FALSE;
  } else {
    assert(mydish == 2);
    dish_two_busy = FALSE;
  }  
  
  P(mutex);
  if(wait_to_finish == FALSE) {
    kprintf("*** Mouse %ld finished eating last.\n", id); /* done. */
    // sync check to make sure there is only one last cat
    first_mouse = TRUE;
    V(mice_queue);
    V(dish_mutex); // the last cat lets the next two in
  } else {
    wait_to_finish = FALSE;
    kprintf("*** Mouse %ld finished eating first.\n", id); /* done. */
  }
  V(mutex);

  mice_eating--;
  kprintf("<<< Mouse %ld left the kitchen.\n", id); /* cat name */
}


/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 */

static
void
catsem(void* unused, unsigned long cat_id)
{
	
  (void)unused;
  (void)cat_id;  
  
  int i;
  
  for (i = 0; i < MAXEAT; i++) {
    clocksleep(random() % MAXTIME);
    hungry_cat(cat_id);
  }
  
  // Signal parent thread that we are done
  V(full);
}

/*
 * mousesem()
 *
 * Arguments:
 *  void * unusedpointer: currently unused.
 *  unsigned long mousenumber: holds the mouse identifier from 0 to NMICE - 1.
 *
 * Returns:
 *  nothing.
 *
 */

static
void
mousesem(void * unused, unsigned long mouse_id)
{
  
  (void) unused;
  (void) mouse_id;
  int i;
  
  for (i = 0; i < MAXEAT; i++) {
    clocksleep(random() % MAXTIME);
    hungry_mouse(mouse_id);
  }
  
  // Signal parent thread that we are done
  V(full);
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs, char ** args) {

  (void) nargs;
  (void) args;

  int index, error, threads_spawned;
 
  full = sem_create("full", 0); 
  done = sem_create("done", 0);
  mutex = sem_create("kitchen", 1);
  dish_mutex = sem_create("dish", 1);
  cats_queue = sem_create("queue of cats", 1);
  mice_queue = sem_create("queue of mice", 1);
  
  all_dishes_available = TRUE;
  cats_wait_count = 0;
  mice_wait_count = 0;
  cat_magic_number = 0xDEADBEEF;
  mouse_magic_number = 0xDEADBEEF;
  first_cat = TRUE;
  first_mouse = TRUE;
  mice_eating = 0;
  cats_eating = 0;
  done_eating = FALSE;
  threads_spawned = 0;
  dish_one_busy = FALSE;
  dish_two_busy = FALSE; 
  wait_to_finish = FALSE;
  /*
   * Start NCATS catsem() threads.
   */
  
  for (index = 0; index < NCATS; index++) {
    error = thread_fork("catsem Thread", NULL, index, catsem, NULL);
    threads_spawned++;
  
    if (error) {
      panic("catsem: thread_fork failed: %s\n", strerror(error));
    }
  }
  
  /*
   * Start NMICE mousesem() threads.
   */
  
  for (index = 0; index < NMICE; index++) {
    error = thread_fork("mousesem Thread", NULL, index, mousesem, NULL);
    threads_spawned++;
  
    if (error) {
      panic("mousesem: thread_fork failed: %s\n", strerror(error));
    }
  }
  
  kprintf("Made %d threads\n", threads_spawned);

  for (index = 0; index < threads_spawned; index++) {
    P(full);
  }
  
  kprintf("All threads done.\n");

return 0;
}


/*
 * End of catsem.c
 */
