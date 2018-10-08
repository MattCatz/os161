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
#define MAXTIME 1
#define MAXEAT 10


static struct semaphore *done;
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

/*
 *
 * Function Definitions
 *
 */
 
void hungry_cat(long unsigned id) {
  int mydish;

  kprintf("Cat %ld got hungry\n",id);
  assert(mutex->count == 1 || mutex->count == 0);
  P(mutex); // cats get kitchen
    
  if (all_dishes_available == TRUE) {
    all_dishes_available = FALSE;
    kprintf("Cat %ld is first\n", id);
    V(cats_queue); // let the first cat in
    //assert(cats_queue->count == 1);
  }
  
  cats_wait_count++;
  V(mutex);  // each cat/mouse lets the next one in
             // The last one leaves the lock for the first cat/mouse
  
  P(cats_queue);
  // at this point everyone but the first cat should be in the queue.
  if (first_cat == TRUE) {
    first_cat = FALSE;
    // sync check that there is only one first cat
    assert(0xDEADBEEF == cat_magic_number++);
    if (cats_wait_count > 0) {
      V(cats_queue);
    }
  }
  V(cats_queue);
  
  kprintf("Cat %ld in the kitchen.\n", id); /* cat name */
  
  P(dish_mutex); /*protect shared variables*/
    if (dish_one_busy  == FALSE) {
      dish_one_busy = TRUE;
      mydish = 1;
    } else {
      assert(dish_two_busy == FALSE);
      dish_two_busy = TRUE;
      mydish = 2;
    }
  
  kprintf("Cat %ld eating bowl %d.\n", id, mydish); /* cat name */
  clocksleep(random() % MAXTIME); /* enjoys food */
  kprintf("Cat %ld finished eating.\n", id); /* done. */
  assert(mice_eating == 0);

    if (mydish == 1) {
      dish_one_busy = FALSE;
    } else {
      assert(mydish == 2);
      dish_two_busy = FALSE;
    }

  P(mutex);
  if(mydish == 1 && !dish_two_busy) {
    kprintf("Cat %ld is last\n", id);
    // sync check to make sure there is only one last cat
    assert(0xDEADBEEF == --cat_magic_number);
    all_dishes_available = TRUE;
    first_cat = TRUE;
    cat_magic_number = 0xDEADBEEF;
    done_eating = FALSE;
  }
  V(mutex);
  V(dish_mutex);
}

void hungry_mouse(unsigned long id) {
  kprintf("Mouse %ld got hungry, mutex entering %d\n",id,  mutex->count);
  assert(mutex->count == 1 || mutex->count == 0);
  P(mutex); // Mice get kitchen
    
  if (all_dishes_available == TRUE) {
    all_dishes_available = FALSE;
    kprintf("Mouse %ld is first\n", id);
    V(mice_queue); // let the first cat in
    //assert(cats_queue->count == 1);
  }
  
  mice_wait_count++;
  V(mutex);  // each cat/mouse lets the next one in
             // The last one leaves the lock for the first cat/mouse
  
  P(mice_queue);
  // at this point everyone but the first cat should be in the queue.
  
  if (first_mouse == TRUE) {
    // sync check that there is only one first mouse
    assert(0xDEADBEEF == mouse_magic_number++);
    first_mouse = FALSE;
    if (mice_wait_count > 0) {
      V(mice_queue);
    }
  }
  
  kprintf("Mouse %ld in the kitchen.\n", id); /*cat name */
  
  P(dish_mutex); /*protect shared variables*/
  P(mice_queue);
  mice_eating++;
  V(mice_queue);
  kprintf("Mouse %ld eating.\n", id); /* cat name */
  clocksleep(1); /* enjoys food */
  kprintf("Mouse %ld finished eating.\n", id); /* done. */
  
  P(mice_queue); 
  mice_eating--;
  mice_wait_count--;
  V(mice_queue);
  V(dish_mutex);
  
  kprintf("Mouse %ld done, remaining mice %d\n", id, cats_wait_count);
 
  P(mutex); 
  P(mice_queue);
  if(mice_wait_count == 0) {
    kprintf("Mouse %ld is last mutex %d\n", id, mutex->count);
    // sync check to make sure there is only one last cat
    assert(0xDEADBEEF == --mouse_magic_number);
    all_dishes_available = TRUE;
    first_mouse = TRUE;
    mouse_magic_number = 0xDEADBEEF;
  }
  V(mice_queue);
  V(mutex);
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
  V(done);
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
  V(done);
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
  
  done = sem_create("done", 0);
  mutex = sem_create("kitchen", 1);
  dish_mutex = sem_create("dish", 2);
  cats_queue = sem_create("queue of cats", 0);
  mice_queue = sem_create("queue of mice", 0);
  
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
  
  for (index = 0; index < 0; index++) {
    error = thread_fork("mousesem Thread", NULL, index, mousesem, NULL);
    threads_spawned++;
  
    if (error) {
      panic("mousesem: thread_fork failed: %s\n", strerror(error));
    }
  }
  
  kprintf("Made %d threads\n", threads_spawned);

  for (index = 0; index < threads_spawned; index++) {
    P(done);
  }
  
  kprintf("All threads done.\n");

return 0;
}


/*
 * End of catsem.c
 */
