head	1.10;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.9
	V1_1:1.6
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.10
date	94.12.21.01.20.11;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.12.12.22.45.15;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.12.10.19.13.41;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.12.09.06.17.22;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.09.17.14.20;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.25.21.23.38;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.23.18.22.36;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.02.15.18.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.01.15.44.07;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.05.55;	author vandys;	state Exp;
branches;
next	;


desc
@Machine-dependent process management stuff
@


1.10
log
@Catch null root page tables, which otherwise shut us down
@
text
@/*
 * machproc.c
 *	Machine-dependent parts of process handling
 */
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/vas.h>
#include <sys/boot.h>
#include <sys/param.h>
#include <sys/percpu.h>
#include <mach/vm.h>
#include <mach/tss.h>
#include <mach/gdt.h>
#include <sys/assert.h>

extern void retuser(), reload_dr(struct dbg_regs *);

/*
 * dup_stack()
 *	Duplicate stack during thread fork
 *
 * "f" is provided to give a starting point for the new thread.
 * Unlike a true fork(), a thread fork gets its own stack within
 * the same virtual address space, and therefore can't run with a
 * copy of the existing stack image.  So we just provide a PC value
 * to start him at, and he runs with a clean stack.
 */
void
dup_stack(struct thread *old, struct thread *new, voidfun f)
{
	ASSERT_DEBUG(old->t_uregs, "dup_stack: no old");

	/*
	 * Calculate location of kernel registers on new stack.
	 * Copy over old to new.
	 */
	new->t_uregs = (struct trapframe *)(
		(new->t_kstack + KSTACK_SIZE) -
		sizeof(struct trapframe));
	bcopy(old->t_uregs, new->t_uregs, sizeof(struct trapframe));

	/*
	 * A thread fork moves to a new, empty stack.  A process
	 * fork has a copy of the stack at the same virtual address,
	 * so the stack location doesn't have to be updated.
	 */
	if (f) {
		new->t_uregs->ebp =
		new->t_uregs->esp =
		 (ulong)(new->t_ustack + UMINSTACK) - sizeof(long);
		new->t_uregs->eip = (ulong)f;
	}

	/*
	 * New entity returns with 0 value; ESP is one lower so that
	 * the resume() path has a place to write its return address.
	 * This simulates the normal context switch mechanism of
	 * setjmp/longjmp.
	 */
	new->t_kregs->eip = (ulong)retuser;
	new->t_kregs->ebp = (ulong)(new->t_uregs);
	new->t_kregs->esp = (new->t_kregs->ebp) - sizeof(ulong);
	new->t_uregs->eax = 0;

	/*
	 * Now that we're done with setup, flag that he's not in
	 * kernel mode.  New processes vector pretty directly into
	 * user mode.
	 */
	new->t_uregs = 0;
}

/*
 * resume()
 *	Jump via the saved stack frame
 *
 * This works because in VSTa all stacks are at unique virtual
 * addresses.
 */
void
resume(void)
{
	struct thread *t = curthread;
	struct proc *p = t->t_proc;
	extern struct tss *tss;

#ifdef PROC_DEBUG
	/*
	 * If we've just switched from a debugged process, or we're
	 * switching into a debugged process, fiddle the debug
	 * registers.
	 */
	if ((cpu.pc_flags & CPU_DEBUG) || p->p_dbgr.dr7) {
		/*
		 * dr7 in a non-debugging process will be 0, so this
		 * covers both cases.  We must mask against preemption
		 * so that the debug registers match our flag.
		 */
		cli();
		reload_dr(&p->p_dbgr);
		if (p->p_dbgr.dr7) {
			cpu.pc_flags |= CPU_DEBUG;
		} else {
			cpu.pc_flags &= ~CPU_DEBUG;
		}
		sti();
	}
#endif

	/*
	 * Make kernel stack come in on our own stack now.  This
	 * isn't used until we switch out to user mode, at which
	 * time our stack will always be empty.
	 * XXX esp is overkill; only esp0 should ever be used.
	 */
	tss->esp0 = tss->esp = (ulong)
		((char *)(t->t_kstack) + KSTACK_SIZE);

	/*
	 * Switch to root page tables for the new process
	 */
	ASSERT_DEBUG(p->p_vas->v_hat.h_cr3 != 0, "resume: cr3 NULL");
	set_cr3(p->p_vas->v_hat.h_cr3);

	/*
	 * Warp out to the context
	 */
	longjmp(t->t_kregs, 1);
}

/*
 * boot_regs()
 *	Set up registers for a boot task
 */
void
boot_regs(struct thread *t, struct boot_task *b)
{
	struct trapframe *u;

	/*
	 * They need to be cleared in t_uregs so it won't look
	 * like a re-entered user mode on our first trap.
	 */
	t->t_uregs = 0;
	u = (struct trapframe *)
		((t->t_kstack + KSTACK_SIZE) - sizeof(struct trapframe));

	/*
	 * Set up user frame to start executing at the boot
	 * task's EIP value.
	 */
	bzero(u, sizeof(struct trapframe));
	u->ecs = GDT_UTEXT|PRIV_USER;
	u->eip = b->b_pc;
	u->esds = ((GDT_UDATA|PRIV_USER) << 16) | GDT_UDATA|PRIV_USER;
	u->ess = GDT_UDATA|PRIV_USER;

	/*
	 * Leave a 0 on the stack to indicate to crt0 that there
	 * are no args.  Leave an extra word so that even once
	 * it has been popped the esp register will always remain
	 * within the stack region.
	 */
	u->ebp =
	u->esp = (USTACKADDR+UMINSTACK) - 2*sizeof(ulong);
	u->eflags = F_IF;

	/*
	 * Set kernel frame to point to the trapframe from
	 * which we'll return.  A jmp_buf uses one word below
	 * the stack frame we build, where the EIP is placed
	 * for the return.
	 */
	bzero(t->t_kregs, sizeof(t->t_kregs));
	t->t_kregs->eip = (ulong)retuser;
	t->t_kregs->ebp = (ulong)u;
	t->t_kregs->esp = (t->t_kregs->ebp) - sizeof(ulong);
}

/*
 * set_execarg()
 *	Pass an argument back to a newly-exec()'ed process
 *
 * For i386, we push it on the stack.
 */
void
set_execarg(struct thread *t, void *arg)
{
	struct trapframe *u = t->t_uregs;

	ASSERT_DEBUG(u, "set_execarg: no user frame");
	u->esp -= sizeof(void *);
	(void)copyout(u->esp, &arg, sizeof(arg));
}

/*
 * reset_uregs()
 *	Reset the user's registers during an exec()
 */
void
reset_uregs(struct thread *t, ulong entry_addr)
{
	struct trapframe *u = t->t_uregs;

	u->ebp =
	u->esp = (USTACKADDR+UMINSTACK) - sizeof(ulong);
	u->eflags = F_IF;
	u->eip = entry_addr;
}

#ifdef PROC_DEBUG
/*
 * single_step()
 *	Control state of single-stepping of current process (user mode)
 */
void
single_step(int start)
{
	struct trapframe *u = curthread->t_uregs;

	if (start) {
		u->eflags |= F_TF;
	} else {
		u->eflags &= ~F_TF;
	}
}

/*
 * set_break()
 *	Set/clear a code breakpoint at given address (user mode)
 *
 * Returns 0 for success, 1 for failure
 */
int
set_break(ulong addr, int set)
{
	uint x;
	struct dbg_regs *d = &curthread->t_proc->p_dbgr;

	/*
	 * Sanity
	 */
	if (addr == 0) {
		return(1);
	}

	/*
	 * Convert to user-linear address, get current status
	 */
	addr |= 0x80000000;

	/*
	 * Clear
	 */
	if (!set) {
		/*
		 * Scan for the register which matches the
		 * named linear address
		 */
		for (x = 0; x < 4; ++x) {
			if (d->dr[x] == addr) {
				break;
			}
		}

		/*
		 * If didn't find, error out
		 */
		if (x >= 4) {
			return(1);
		}

		/*
		 * Clear it, and re-load our debug registers
		 */
		d->dr7 &= ~(3 << (x*2));
		d->dr[x] = 0;
		cli();
		reload_dr(d);
		if (d->dr7 == 0) {
			cpu.pc_flags &= ~CPU_DEBUG;
		}
		sti();
		return(0);
	}

	/*
	 * Set--find open slot
	 */
	for (x = 0; x < 4; ++x) {
		if (d->dr[x] == 0) {
			break;
		}
	}

	/*
	 * Bomb if they're all filled up
	 */
	if (x >= 4) {
		return(1);
	}

	/*
	 * Set up, and put into hardware
	 */
	d->dr7 |= (0x2 << (x*2));
	d->dr[x] = addr;
	cli();
	reload_dr(d);
	cpu.pc_flags |= CPU_DEBUG;
	sti();
	return(0);
}

/*
 * getreg()
 *	Get value of register, based on index
 */
long
getreg(long index)
{
	if ((index < 0) || (index >= NREG)) {
		return(-1);
	}
	return(*((long *)(curthread->t_uregs) + index));
}

/*
 * setreg()
 *	Set value of register, based on index
 *
 * Be appropriately paranoid, especially with things like the flags.
 * Return 1 on error, 0 on success.
 */
int
setreg(long index, long value)
{
	struct trapframe *tf;
	ulong *lp;

	if ((index < 0) || (index >= NREG)) {
		return(-1);
	}

	/*
	 * For convenience, point to our register set
	 */
	tf = curthread->t_uregs;
	lp = (ulong *)((long *)tf + index);

	/*
	 * Flags--he can fiddle some, but protect the sensitive ones
	 */
	if (lp == &tf->eflags) {
		static const int priv =
			(F_TF|F_IF|F_IOPL|F_NT|F_RF|F_VM);

		/*
		 * Do not let him touch privved bits in either
		 * direction.  Others he may set at will.
		 */
		tf->eflags = (tf->eflags & priv) | (value & ~priv);
	}

	/*
	 * He can rewrite all the regular registers, but nothing else.
	 */
	if ((lp != &tf->eax) && (lp != &tf->ebx) && (lp != &tf->ecx) &&
		(lp != &tf->edx) && (lp != &tf->edi) && (lp != &tf->esi) &&
		(lp != &tf->esp) && (lp != &tf->ebp)) {
	    return(1);
	}
	*lp = value;
	return(0);
}

#endif /* PROC_DEBUG */
@


1.9
log
@Fix support for debug registers, optimize a bit
@
text
@d122 1
@


1.8
log
@Oops, wrong thing to take addr of
@
text
@d16 1
a16 1
extern void retuser();
d83 2
d87 23
d117 1
a117 1
		((char *)(curthread->t_kstack) + KSTACK_SIZE);
d122 1
a122 1
	set_cr3(curthread->t_proc->p_vas->v_hat.h_cr3);
d127 1
a127 1
	longjmp(curthread->t_kregs, 1);
d275 1
a275 1
		d->dr7 &= ~(3 << x);
d277 1
d279 4
d305 1
a305 1
	d->dr7 |= (3 << x);
d307 1
d309 2
@


1.7
log
@Add ptrace() support
@
text
@d291 1
a291 1
	return(*((long *)(&curthread->t_uregs) + index));
@


1.6
log
@Fix comment
@
text
@d184 160
@


1.5
log
@Make sure we leave room for our 0 argument which tells
crt0 that this is a boot proc.
@
text
@d2 1
a2 1
 * proc.c
@


1.4
log
@Add some routines to initialize a process' state after
exec()'ing a new file.
@
text
@d133 4
a136 2
	 * esp must lie within data segment, so point at last
	 * word at top of stack.
d139 1
a139 1
	u->esp = (USTACKADDR+UMINSTACK) - sizeof(ulong);
@


1.3
log
@Tweak stack duplication so it'll work for both tfork() and fork()
@
text
@d151 31
@


1.2
log
@Get tfork() working
@
text
@d32 5
d41 12
a52 2
	new->t_uregs->ebp =
	new->t_uregs->esp = (ulong)(new->t_ustack + UMINSTACK) - sizeof(long);
d55 1
a55 1
	 * New thread returns with 0 value; ESP is one lower so that
a62 3
	if (f) {
		new->t_uregs->eip = (ulong)f;
	}
d64 7
@


1.1
log
@Initial revision
@
text
@d21 6
d29 1
a29 1
dup_stack(struct thread *old, struct thread *new)
d32 3
a34 2
	new->t_uregs = (struct trapframe *)new->t_kstack -
		sizeof(struct trapframe);
d36 2
a37 2
	new->t_uregs->esp = (ulong)(new->t_ustack + UMINSTACK);
	new->t_kregs->esp = (ulong)(new->t_uregs);
d40 4
a43 2
	 * New thread returns with 0 value; retuser() also does some
	 * fixups for a new process.
d45 6
a51 1
	new->t_kregs->eip = (ulong)retuser;
d101 1
a101 1
		((t->t_kstack + NBPG) - sizeof(struct trapframe));
@
