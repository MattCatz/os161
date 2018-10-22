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

/*
 * Used for boolean logic.
 */
#define TRUE 1
#define FALSE 0

/*
 * The maximum time that a cat/mouse will wait to eat and and take to eat.
 */
#define MAXTIME 4

/*
 * Number of times a cat/mouse will eat.
 */
#define MAXEAT 10

/*
 * Please see for catmousesem instanciation values and explination of use.
 */
static struct semaphore *kitchen;
static struct semaphore *full;
static struct semaphore *mutex;
static struct semaphore *dish_mutex;
static struct semaphore *cats_queue;
static struct semaphore *mice_queue;
static volatile int cats_wait_count;
static volatile int mice_wait_count;
static volatile unsigned int cat_magic_number;
static volatile unsigned int mouse_magic_number;
static volatile int first_cat;
static volatile int first_mouse;
static volatile int mice_eating;
static volatile int dish_one_busy;
static volatile int dish_two_busy;
static volatile int wait_to_finish;

/*
 * Used to identify thread type
 */
typedef enum thread_type {Cat, Mouse} Thread_type;

/*
 *
 * Function Definitions
 *
 */

/*
 * Used when an animal eats. Done this was because the logic is very similar for both
 * thread types.
 */
void eat (Thread_type t, long unsigned id) {
   int mydish;
   // Protect our dish variables
   P(dish_mutex);
   if (dish_one_busy == FALSE) {
      dish_one_busy = TRUE;
      mydish = 1;
   } else {
      assert(dish_two_busy == FALSE);
      dish_two_busy = TRUE;
      mydish = 2;
   }
   V(dish_mutex);

   // The only big difference
   if (t == Cat) {
      kprintf("*** Cat %ld eating bowl %d.\n", id, mydish);
      clocksleep(random() % MAXTIME); /* enjoys food */
      assert(mice_eating == 0);
   } else {
      assert(t == Mouse);
      kprintf("*** Mouse %ld eating bowl %d.\n", id, mydish);
      clocksleep(random() % MAXTIME); /* enjoys food */
   }

   // Protect our dish variables
   P(dish_mutex);
   if (mydish == 1) {
      dish_one_busy = FALSE;
   } else {
      assert(mydish == 2);
      dish_two_busy = FALSE;
   }
   V(dish_mutex);
}

/*
 * When a cat gets hungry this function is called. It holds the core logic
 * for cat threads.
 */
void hungry_cat (long unsigned id) {
   kprintf("    Cat %ld got hungry\n",id);

   P(mutex);
   cats_wait_count++;
   V(mutex);

   P(cats_queue); // cats get in line
   P(mutex);
   cats_wait_count--;
   V(mutex);

   if (first_cat == TRUE) {
      P(mutex);
      first_cat = FALSE;
      V(mutex);
      kprintf("    Cat %ld first in the cat queue.\n", id);

      P(kitchen); /*Let the first cat wait for the kitchen*/

      kprintf(">>> Cat %ld got into kitchen.\n", id);

      // At this point Cats control the kitchen so lets let in another
      // cat since there are two bowl.
      if (cats_wait_count > 0) {
         kprintf("... Letting a cat \n");
         V(cats_queue); // first cat brings in the next cat in the queue
         wait_to_finish = TRUE;
      }

   } else {
      kprintf(">>> Cat %ld let in the kitchen.\n", id);
   }

   // At this point cat(s) are free to eat
   eat(Cat,id);

   P(mutex);
   // This is flag is set when we should wait to clean-up for the next kitchen
   // visitor
   if(wait_to_finish == FALSE) {
      kprintf("*** Cat %ld finished eating last.\n", id); /* done. */
      first_cat = TRUE;
      V(cats_queue);
      V(kitchen);
   } else {
      // if there are two cat then when the first one leaves it will set this
      // to tell the other cat that he has to clean up
      wait_to_finish = FALSE;
      kprintf("*** Cat %ld finished eating first.\n", id); /* done. */
   }
   kprintf("<<< Cat %ld left the kitchen.\n", id); /* cat name */
   V(mutex);
}

void hungry_mouse (unsigned long id) {
   kprintf("    Mouse %ld got hungry\n",id);

   P(mutex);
   mice_wait_count++;
   V(mutex);

   P(mice_queue); // mice get in line
   P(mutex);
   mice_wait_count--;
   V(mutex);

   if (first_mouse == TRUE) {
      P(mutex);
      first_mouse = FALSE;
      V(mutex);
      kprintf("    Mouse %ld first in mouse queue.\n", id);

      P(kitchen); // First mouse tries to get the kitchen

      kprintf(">>> Mouse %ld got into kitchen.\n", id);

      if (mice_wait_count > 0) {
         kprintf("... Letting in other mouse \n");
         V(mice_queue); // first mouse brings the next cat in the queue
         wait_to_finish = TRUE;
      }

   } else {
      kprintf(">>> Mouse %ld let in the kitchen.\n", id);
   }
   P(mutex);
   mice_eating++;
   V(mutex);

   // At this point mice/mouse are free to eat
   eat(Mouse,id);

   P(mutex);
   // This is flag is set when we should wait to clean-up for the next kitchen
   // visitor
   mice_eating--;
   if(wait_to_finish == FALSE) {
      kprintf("*** Mouse %ld finished eating last.\n", id);
      first_mouse = TRUE;
      V(mice_queue);
      V(kitchen);
   } else {
      wait_to_finish = FALSE;
      kprintf("*** Mouse %ld finished eating first.\n", id);
   }
   kprintf("<<< Mouse %ld left the kitchen.\n", id); /* cat name */
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

   full = sem_create("full", 0); // Used to signaly that an animal wont eat anymore
   kitchen = sem_create("kitchen", 1); // Holder controlls the kitchen
   mutex = sem_create("mutex", 1); // Used to lock global variables
   dish_mutex = sem_create("dish", 1); // Used to lock dish variables

   /* Used to control the flow of cats/mice into the kithcne. Notice that they start at
      a value of one to let the first cat/mouse try an get control of the kitchen.*/
   cats_queue = sem_create("queue of cats", 1);
   mice_queue = sem_create("queue of mice", 1);


   /* Keeps track of who is in the kitchen */
   cats_wait_count = 0;
   mice_wait_count = 0;

   /* These are used to check that certain parts of code is only
      visited by one cat/mouse */
   cat_magic_number = 0xDEADBEEF;
   mouse_magic_number = 0xDEADBEEF;


   first_cat = TRUE;
   first_mouse = TRUE;

   mice_eating = 0;

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
      P(full); // Wait for all the cat/mice to be full
   }

   kprintf("All threads done.\n");

   return 0;
}


/*
 * End of catsem.c
 */
