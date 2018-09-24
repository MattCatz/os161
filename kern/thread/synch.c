/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <queue.h>
#include <curthread.h>
#include <machine/spl.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}

	lock->owner = NULL;
	DEBUG(DB_THREADS, "Created lock : %s\n", lock->name);	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	assert(lock->owner == NULL);

	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{	
	int spl;

	spl = splhigh();

	assert(lock != NULL);

	DEBUG(DB_THREADS, "Trying to acquire lock : %s\n", lock->name);	
	while (lock->owner != NULL) {
		thread_sleep(lock);
	}

	lock->owner = curthread;
	splx(spl);	
}

void
lock_release(struct lock *lock)
{
	int spl;

	spl = splhigh();
	
	assert(lock != NULL);
	assert(lock_do_i_hold(lock));

	DEBUG(DB_THREADS, "Releasing lock : %s\n", lock->name);	
	lock->owner = NULL;

	thread_wakeup(lock);

	splx(spl);
}

int
lock_do_i_hold(struct lock *lock)
{
	assert(lock != NULL);
	if (lock->owner == curthread) {
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}

	cv->waitlist = q_create(1);
	if (cv->waitlist == NULL) {
		kfree(cv);
		kfree(cv->name);
	}
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);
	assert(cv->name != NULL);
	assert(cv->waitlist != NULL);

	kfree(cv->name);
	q_destroy(cv->waitlist);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int spl;

	assert(cv != NULL);
	assert(lock != NULL);

	spl = splhigh();
		
	lock_release(lock);
	//q_addtail(cv->waitlist, curthread);
	thread_sleep(cv);
	lock_acquire(lock);
	
	splx(spl);	
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int spl;

	assert(cv != NULL);
	assert(lock != NULL);
	assert(cv->waitlist != NULL);

	spl = splhigh();
	//if (!q_empty(cv->waitlist)) {
	//	next = q_remhead(cv->waitlist);
	//	thread_wakeup_next(next);
	//}
	thread_wakeup_next(cv);

	splx(spl);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int spl;

	assert(cv != NULL);
	assert(lock != NULL);
	assert(cv->waitlist != NULL);

	spl = splhigh();

	//while (!q_empty(cv->waitlist)) {
	//	thread_wakeup_next(lock);
	//}

	thread_wakeup(cv);

	splx(spl);
}
