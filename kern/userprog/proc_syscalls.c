#include <types.h>
#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/trapframe.h>

/*
*  You may need the following four head files to
*  implement other process related system calls.
*/
/*
#include <pid.h>
*/

/*
*  Sample implementation of sys_getpid(), which is called in
*  src/kern/arch/mips/mips/syscall.c
*/
int
sys_getpid(pid_t *retval)
{
	*retval = curthread->t_pid;
	return 0;
}

/* Used in sys_fork */
static
void
child_thread(void *vtf, unsigned long junk)
{
        struct trapframe mytf;
        struct trapframe *ntf = vtf;
        (void)junk;
        /*
         * Now copy the trapframe to our stack, so we can free the one
        * that was malloced and use the one on our stack for going to
        * userspace.
        */
        mytf = *ntf;
        kfree(ntf);
        md_forkentry(&mytf);
}

int
sys_fork(struct trapframe * tf, int *retval) {
	struct trapframe *new_tf;
	int result;

	new_tf = kmalloc(sizeof (struct trapframe));
	if (new_tf == NULL) {
		return ENOMEM;
	}

	*new_tf = *tf;
	new_tf->tf_v0 = 0;
	new_tf->tf_a3 = 0;
	new_tf->tf_epc += 4;

	result = thread_fork(curthread->t_name, (void *) new_tf, NULL, child_thread, retval);
	
	if (result != 0) {
		kfree(new_tf);
		return result;
	}

	return 0;
}

