/*
 * sched.c
 *	Routines for scheduling
 *
 * The decision of who to run next is solved in VSTa by placing all
 * threads within nodes organized as a tree.  All sibling nodes compete
 * on a percentage basis under their common parent.  A node which "wins"
 * will either (1) run the process if it's a leaf, or (2) recursively
 * distribute the "won" CPU time among its competing children.
 */
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/percpu.h>
#include <sys/sched.h>
#include <sys/assert.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/fs.h>
#include <alloc.h>

extern ulong random();
extern void idle(void);

lock_t runq_lock;		/* Mutex for scheduling */
struct sched
	sched_rt,		/* Real-time queue */
	sched_cheated,		/* Low-CPU processes given preference */
	sched_bg,		/* Background (lowest priority) queue */
	sched_root;		/* "main" queue */
volatile uint num_run = 0;	/* # SRUN procs waiting */

/*
 * queue()
 *	Enqueue a node below an internal node
 */
static void
queue(struct sched *sup, struct sched *s)
{
	struct sched *hd = sup->s_down;

	if (hd == 0) {
		sup->s_down = s;
		s->s_hd = s->s_tl = s;
	} else {
		s->s_tl = hd->s_tl;
		s->s_hd = hd;
		hd->s_tl = s;
		s->s_tl->s_hd = s;
	}
}

/*
 * dequeue()
 *	Remove element from its queue
 */
static void
dequeue(struct sched *up, struct sched *s)
{
	if (s->s_hd == s) {
		/*
		 * Only guy in queue--clear parent's down pointer
		 */
		up->s_down = 0;
	} else {
		/*
		 * Do doubly-linked deletion
		 */
		s->s_tl->s_hd = s->s_hd;
		s->s_hd->s_tl = s->s_tl;

		/*
		 * Move parent's down pointer if it's pointing to
		 * us.
		 */
		if (up->s_down == s) {
			up->s_down = s->s_hd;
		}
	}
#ifdef DEBUG
	s->s_tl = s->s_hd = 0;
#endif
}

/*
 * preempt()
 *	Go around the list of engines and find best one to preempt
 */
static void
preempt(void)
{
	struct percpu *c, *lowest;
	ushort pri = 99;
	extern void nudge();

	c = nextcpu;
	do {
		if (!(c->pc_flags & CPU_UP)) {
			continue;
		}
		if (c->pc_pri < pri) {
			lowest = c;
			pri = c->pc_pri;
		}
		c = c->pc_next;
	} while (c != nextcpu);
	ASSERT_DEBUG(pri != 99, "preempt: no cpus");
	nextcpu = c->pc_next;
	nudge(c);
}

/*
 * pick_run()
 *	Pick next runnable process from scheduling tree
 *
 * runq lock is assumed held by caller.
 */
static struct sched *
pick_run(struct sched *root)
{
	struct sched *s = root, *s2;

	/*
	 * Walk our way down the tree
	 */
	for (;;) {
		struct sched *pick;
		uint sum, nrun;

		/*
		 * Scan children.  We do this in two passes (ick?).
		 * In the first, we sum up all the pending priorities;
		 * in the second, we pick out the lucky winner to be
		 * run.
		 */
		pick = 0;
		nrun = sum = 0;
		s2 = s;
		do {
			if (s2->s_nrun) {
				pick = s2;
				sum += s2->s_prio;
				nrun += 1;
			}
			s2 = s2->s_hd;
		} while (s2 != s);
		ASSERT_DEBUG(nrun > 0, "pick_run: !nrun");

		/*
		 * If there was only one choice, run with it.  Otherwise,
		 * roll the dice and pick one statistically.
		 */
		if (nrun > 1) {
			sum = random() % sum;
			s2 = s;
			do {
				if (s2->s_nrun) {
					pick = s2;
					if (s2->s_prio >= sum) {
						break;
					}
					sum -= s2->s_prio;
				}
				s2 = s2->s_hd;
			} while (s2 != s);
		}
		ASSERT_DEBUG(pick, "pick_run: !pick");

		/*
		 * Advance down tree to this node.  Done when we find
		 * a leaf node to run.
		 */
		if (pick->s_leaf) {
			s = pick;
			break;
		} else {
			s = pick->s_down;
		}
	}

	/*
	 * We have made our choice.  Remove from tree and update
	 * nrun counts.
	 */
	ASSERT_DEBUG(s->s_leaf, "pick_run: !leaf");
	dequeue(s->s_up, s);
	for (s2 = s; s2; s2 = s2->s_up) {
		s2->s_nrun -= 1;
	}
	return(s);
}

/*
 * savestate()
 *	Dump off our state
 *
 * Returns 1 for the resume()'ed thread, 0 for the caller
 */
inline static int
savestate(struct thread *t)
{
	extern void fpu_disable();

	if (t->t_flags & T_FPU) {
		fpu_disable(t->t_fpu);
		t->t_flags &= ~T_FPU;
	}
	return(setjmp(curthread->t_kregs));
}

/*
 * swtch()
 *	Switch, perhaps to another process
 *
 * swtch() is called when a process has queued itself under a semaphore
 * and now wishes to relinquish the CPU.  This routine picks the next
 * process to run, if any.  If there IS one, it is switched to.  If not,
 * swtch() moves to its idle stack and awaits new work.  The special case
 * of switching back to the current process is recognized and optimized.
 *
 * The use of local variables after the switch to idle_stack() is a little
 * risky; the *values* of the variables is not expected to be preserved.
 * However, the variables themselves are still accessed.  The idle stack
 * is constructed with some room to make this possible.
 *
 * swtch() is called with runq_lock held.
 */
void
swtch(void)
{
	struct sched *s;
	ushort pri;
	struct thread *t = curthread;

	/*
	 * Now that we're going to reschedule, clear any pending preempt
	 * request.  If we voluntarily gave up the CPU, decrement
	 * one point of CPU (over) usage.
	 */
	ASSERT_DEBUG(cpu.pc_nopreempt == 0, "swtch: slept in no preempt");
	if (t && (t->t_state == TS_SLEEP) && (t->t_oink > 0)) {
		t->t_oink -= 1;
	}
	do_preempt = 0;

	for (;;) {
		/*
		 * See if we can find something to run
		 */
		if (sched_rt.s_down) {
			s = sched_rt.s_down;
			dequeue(&sched_rt, s);
			pri = PRI_RT;
			ASSERT_DEBUG(s->s_leaf, "swtch: rt not leaf");
		} else if (sched_cheated.s_down) {
			s = sched_cheated.s_down;
			dequeue(&sched_cheated, s);
			pri = PRI_CHEATED;
		} else if (sched_root.s_nrun > 0) {
			s = pick_run(&sched_root);
			pri = PRI_TIMESHARE;
		} else if (sched_bg.s_down) {
			s = sched_bg.s_down;
			dequeue(&sched_bg, s);
			pri = PRI_BG;
			ASSERT_DEBUG(s->s_leaf, "swtch: bg not leaf");
		} else {
			s = 0;
		}

		/*
		 * Yup, drop out to run it
		 */
		if (s) {
			break;
		}

		/*
		 * Save our current state
		 */
		if (t) {
			if (savestate(t)) {
				/*
				 * This is the code path from being
				 * resume()'ed.
				 */
				return;
			}
		}

		/*
		 * Release lock, switch to idle stack, idle.
		 */
		idle_stack();
		v_lock(&runq_lock, SPL0);
		t = curthread = 0;
		idle();
		p_lock(&runq_lock, SPLHI);
	}

	/*
	 * If we've picked the same guy, don't need to do anything fancy.
	 */
	if (s->s_thread == t) {
		if (pri != PRI_CHEATED) {
			t->t_runticks = RUN_TICKS;
		}
		cpu.pc_pri = pri;
		curthread->t_state = TS_ONPROC;
		v_lock(&runq_lock, SPL0);
		return;
	}

	/*
	 * Otherwise push aside current thread and go with new
	 */
	if (t) {
		if (savestate(t)) {
			return;
		}
	}

	/*
	 * Assign priority.  Do not replenish CPU quanta if they
	 * are here because they are getting preference to continue
	 * their previous allocation.
	 */
	cpu.pc_pri = pri;
	t = curthread = s->s_thread;
	if (pri != PRI_CHEATED) {
		t->t_runticks = RUN_TICKS;
	}
	t->t_state = TS_ONPROC;
	t->t_eng = &cpu;
	idle_stack();
	v_lock(&runq_lock, SPL0);
	resume();
	ASSERT(0, "swtch: back from resume");
}

/*
 * lsetrun()
 *	Version of setrun() where runq lock already held
 */
void
lsetrun(struct thread *t)
{
	struct sched *s = t->t_runq;

	ASSERT_DEBUG(t->t_wchan == 0, "lsetrun: wchan");
	t->t_state = TS_RUN;
	ATOMIC_INC(&num_run);
	if (t->t_flags & T_RT) {

		/*
		 * If thread is real-time, queue to FIFO run queue
		 */
		queue(&sched_rt, s);
		preempt();
	} else if (t->t_flags & T_BG) {

		/*
		 * Similarly for background
		 */
		queue(&sched_bg, s);
	} else if ((t->t_runticks > CHEAT_TICKS) && !t->t_oink) {

		/*
		 * A timeshare process which used little of its
		 * CPU quanta queues preferentially.  Preempt
		 * if the current guy's lower than this.
		 */
		queue(&sched_cheated, s);
		if (cpu.pc_pri < PRI_CHEATED) {
			preempt();
		}
	} else {

		/*
		 * Insert our node into the FIFO circular list
		 */
		ASSERT_DEBUG(s->s_leaf, "lsetrun: !leaf");
		queue(s->s_up, s);

		/*
		 * Bump the nrun count on each node up the tree
		 */
		for ( ; s; s = s->s_up) {
			s->s_nrun += 1;
		}

		/*
		 * XXX we don't have classic UNIX priorities, but would it be
		 * desirable to preempt someone now instead of waiting for
		 * a timeslice?
		 */
	}
}

/*
 * setrun()
 *	Make a thread runnable
 *
 * Enqueues the thread at its node, and flags all nodes upward as having
 * another runnable process.
 *
 * This routine handles its own locking.
 */
void
setrun(struct thread *t)
{
	spl_t s;

	s = p_lock(&runq_lock, SPLHI);
	lsetrun(t);
	v_lock(&runq_lock, s);
}

/*
 * timeslice()
 *	Called when a process might need to timeslice
 */
void
timeslice(void)
{
	p_lock(&runq_lock, SPLHI);
	ATOMIC_DEC(&num_run);
	lsetrun(curthread);
	swtch();
}

/*
 * init_sched2()
 *	Set up a "struct sched" for use
 */
static void
init_sched2(struct sched *s)
{
	s->s_refs = 1;
	s->s_hd = s->s_tl = s;
	s->s_up = 0;
	s->s_down = 0;
	s->s_leaf = 0;
	s->s_prio = PRIO_DEFAULT;
	s->s_nrun = 0;
}

/*
 * init_sched()
 *	One-time setup for scheduler
 */
void
init_sched(void)
{
	/*
	 * Init the runq lock.
	 */
	init_lock(&runq_lock);

	/*
	 * Set up the scheduling queues
	 */
	init_sched2(&sched_rt);
	init_sched2(&sched_bg);
	init_sched2(&sched_root);
	init_sched2(&sched_cheated);
}

/*
 * sched_thread()
 *	Create a new sched node for a thread
 */
struct sched *
sched_thread(struct sched *parent, struct thread *t)
{
	struct sched *s;

	s = MALLOC(sizeof(struct sched), MT_SCHED);
	p_lock(&runq_lock, SPLHI);
	s->s_up = parent;
	s->s_thread = t;
	s->s_prio = PRIO_DEFAULT;
	s->s_leaf = 1;
	s->s_nrun = 0;
	parent->s_refs += 1;
	v_lock(&runq_lock, SPL0);
	return(s);
}

/*
 * sched_node()
 *	Add a new internal node to the tree
 *
 * Inserts the internal node into the static tree structure; adds
 * a reference to the parent node.
 */
struct sched *
sched_node(struct sched *parent)
{
	struct sched *s;

	s = MALLOC(sizeof(struct sched), MT_SCHED);
	p_lock(&runq_lock, SPLHI);
	s->s_up = parent;
	s->s_down = 0;
	s->s_prio = PRIO_DEFAULT;
	s->s_leaf = 0;
	s->s_nrun = 0;
	s->s_refs = 0;
	queue(parent, s);
	parent->s_refs += 1;
	v_lock(&runq_lock, SPL0);
	return(s);
}

/*
 * free_sched_node()
 *	Free scheduling node, updating parent if needed
 */
void
free_sched_node(struct sched *s)
{
	(void)p_lock(&runq_lock, SPLHI);

	/*
	 * De-ref parent
	 */
	s->s_up->s_refs -= 1;

	/*
	 * If not a leaf, this node is linked under the parent.  Remove
	 * it.
	 */
	if (s->s_leaf == 0) {
		dequeue(s->s_up, s);
	}

	v_lock(&runq_lock, SPL0);

	/*
	 * Free the node
	 */
	FREE(s, MT_SCHED);
}

/*
 * sched_prichg()
 *	Change the scheduling priority of the current thread
 */
static int
sched_prichg(int new_pri)
{
	spl_t s;
	struct thread *t;
	int x;

	if ((new_pri == PRI_RT) && !isroot()) {
		/*
		 * isroot() will set EPERM for us
		 */
		return(-1);
	}

	s = p_lock(&runq_lock, SPLHI);

	t->t_flags &= ~T_BG;
	t->t_flags &= ~T_RT;
	if (new_pri == PRI_BG) {
		t->t_flags |= T_BG;
	}
	if (new_pri == PRI_RT) {
		t->t_flags |= T_RT;
	}
	if (cpu.pc_pri > new_pri) {
		/*
		 * If we've dropped in priority nudge our engine
		 */
		nudge(t->t_eng);
	}

	cpu.pc_pri = new_pri;
	v_lock(&runq_lock, s);
	return(0);
}

/*
 * sched_op()
 *	Handle scheduling operations requested by user-space code
 */
int
sched_op(int op, int arg)
{
	/*
	 * Look at what we've been requested to do
	 */
	switch(op) {
	case SCHEDOP_SETPRIO:	/* Set process priority */
		/*
		 * Range check the priority specified as the
		 * argument.  An "idle" priority means just yield
		 * and retry for the CPU.
		 */
		if ((arg < PRI_IDLE) || (arg > PRI_RT)) {
			break;
		}
		if (arg == PRI_IDLE) {
			timeslice();
			return(0);
		}
		return(sched_prichg(arg));

	case SCHEDOP_GETPRIO:
		/*
		 * Return the current priority
		 */
		return((curthread->t_flags & T_BG) ? PRI_BG :
			(curthread->t_flags & T_RT) ? PRI_RT : PRI_TIMESHARE);

	default:
		break;
	}
	return(err(EINVAL));
}
