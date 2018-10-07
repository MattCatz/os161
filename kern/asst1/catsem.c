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
#define False 0
#define MAXTIME 10
#define MAXEAT 10


volatile bool all_dishes_available;
semaphore *done;
semaphore *mutex; // Kitchen
semaphore *dish_mutex;
semaphore *cats_queue;
semaphore *mice_queue;
volatile int cats_wait_count;
volatile int mice_wait_count;
volatile int cat_magic_number;
volatile int mouse_magic_number;

/*
 *
 * Function Definitions
 *
 */
 
void hungry_cat() {
  assert(mutex->count == 1 || mutex->count == 0);
  P(mutex); // cats get kitchen
    
  if (all_dishes_available == TRUE) {
    // With the mutex at an intial value of 1 garuntees no race condition for all_dishes_available
    all_dishes_available = FALSE;
    V(cat_queue); // let the first cat in
    assert(cat_queue->count == 1);
  }
  
  cats_wait_count++;
  V(mutex);  // each cat/mouse lets the next one in
             // The last one leaves the lock for the first cat/mouse
  
  P(cat_queue);
  // at this point everyone but the first cat should be in the queue.
  
  if (first_cat == true) {
    // sync check that there is only one first cat
    assert(0xDEADBEEF == cat_magic_number++);
    first_cat = false;
    P(mutex); // We either get this first try or well have to wait for the last mouse to signal us.
    // Hold the door open for all cats
    for (int i = cat_wait_count; i > 0; i--) {
      V(cat_queue);
    }
  }
  
  kprintf(“Cat in the kitchen.\n”); /*cat name */
  
  P(dish_mutex); /*protect shared variables*/

  kprint(“Cat eating.\n”); /* cat name */
  clocksleep(1); /* enjoys food */
  kprint(“Finish eating.\n”); /* done. */
  assert(mice_eating == 0);
  
  cats_wait_count--;
  V(dish_mutex);
  V(cat_queue);
  
  assert(cats_wait_count >= 0);
  
  if(cats_wait_count == 0) {
    // sync check to make sure there is only one last cat
    assert(0xDEADBEEF == --cat_magic_number);
    all_dishes_available = TRUE;
    V(mutex);
  }
}

void hungry_mouse() {
  assert(mutex->count == 1 || mutex->count == 0);
  P(mutex); // get in mouse line
  
  if (all_dishes_available == TRUE) {
    // With the mutex at an intial value of 1 garuntees no race condition for all_dishes_available
    all_dishes_available = FALSE;
    V(mouse_queue); // let the first cat in
    assert(mouse_queue->count == 1);
  }
  
  mouse_wait_count++;
  V(mutex);  // each cat/mouse lets the next one in
             // The last one leaves the lock for the first cat/mouse
  
  P(mouse_queue);
  // at this point everyone but the first cat should be in the queue.
  
  if (first_mouse == true) {
    // Synch check to make sure there is only one first mouse
    assert(0xDEADBEEF == mouse_magic_number++);
    first_mouse = false;
    P(mutex); //We either get this first try or well have to wait for the last cat to signal us.
    // Hold the door open for all cats
    for (int i = mouse_wait_count; i > 0; i--) {
      V(mouse_queue);
    }
  }
  
  kprintf(“Mouse in the kitchen.\n”); /*cat name */
  
  P(dish_mutex); /*protect shared variables*/
  mice_eating++;
  kprint(“mouse eating.\n”); /* cat name */
  clocksleep(1); /* enjoys food */
  kprint(“Finish eating.\n”); /* done. */
  mice_eating--;
  cats_wait_count--;
  V(dish_mutex);
  V(cat_queue);
  
  assert(mouse_wait_count >= 0);
  
  if(mouse_wait_count == 0) {
    // sync check to make sure there is only one last mouse
    assert(0xDEADBEEF == --mouse_magic_number);
    all_dishes_available = TRUE;
    V(mutex);
  }
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
catsem(semaphore * sem_cat, unsigned long cat_id)
{
  assert(sem_cat);
  assert(cat_id > -1);
  
  int i;
  
  for (i = 0; i < MAXEAT; i++) {
    clocksleep(random() % MAXTIME);
    hungry_cat();
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
mousesem(semaphore * sem_mouse, unsigned long mouse_id)
{
  assert(sem_cat);
  assert(cat_id > -1);
  
  int i;
  
  for (i = 0; i < MAXEAT; i++) {
    clocksleep(random() % MAXTIME);
    hungry_mouse();
  }
  
  // Signal parent thread that we are done
  V(done)
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

  int index, error, threads_spawned;
  
  sem_create(done, 0);
  sem_create(mutex, 1);
  sem_create(dish_mutex, NFOODBOWLS);
  sem_create(cats_queue, 0);
  sem_create(mice_queue, 0);
  
  all_dishes_available = true;
  cats_wait_count = 0;
  mice_wait_count = 0;
  cat_magic_number = 0xDEADBEEF;
  mouse_magic_number = 0xDEADBEEF;
  
  /*
   * Start NCATS catsem() threads.
   */
  
  for (index = 0; index < NCATS; index++) {
    error = thread_fork("catsem Thread", sem_cat, index, catsem, NULL);
    threads_spawned++;
  
    if (error) {
      panic("catsem: thread_fork failed: %s\n", strerror(error));
    }
  }
  
  /*
   * Start NMICE mousesem() threads.
   */
  
  for (index = 0; index < NMICE; index++) {
    error = thread_fork("mousesem Thread", sem_mouse, index, mousesem, NULL);
    threads_spawned++;
  
    if (error) {
      panic("mousesem: thread_fork failed: %s\n", strerror(error));
    }
  }
  
  for (int i = 0; i < threads_spawned; i++) {
    P(wait);
  }
  
  kprintf("All threads done.\n");

return 0;
}


/*
 * End of catsem.c
 */
