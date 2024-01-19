head	1.23;
access;
symbols
	V1_3_1:1.17
	V1_3:1.16
	V1_2:1.13
	V1_1:1.10
	V1_0:1.8;
locks; strict;
comment	@ * @;


1.23
date	95.01.05.17.00.17;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	95.01.05.01.38.18;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	94.12.21.05.26.25;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	94.10.05.17.57.46;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	94.08.30.22.12.18;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	94.05.21.21.44.32;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.04.19.03.16.05;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.04.06.18.41.07;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.03.15.21.58.13;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.03.08.23.21.13;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.12.27.22.37.25;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.12.14.23.13.53;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.12.09.06.17.13;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.10.02.01.08.46;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.08.18.05.26.30;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.06.30.19.55.39;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.04.23.22.42.51;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.09.17.14.35;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.13.01.32.48;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.12.19.44.07;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.09.17.11.32;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.15.10.54;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.07.54;	author vandys;	state Exp;
branches;
next	;


desc
@Front-line trap handling, both interrupts and exceptions
@


1.23
log
@Restructure switch() statements to allow encoding of dense
jump tables
@
text
@/*
 * trap.c
 *	Trap handling for i386 uP
 */
#include <sys/proc.h>
#include <sys/percpu.h>
#include <sys/thread.h>
#include <sys/wait.h>
#include <sys/fs.h>
#include <sys/malloc.h>
#include <mach/trap.h>
#include <mach/gdt.h>
#include <mach/tss.h>
#include <mach/vm.h>
#include <mach/icu.h>
#include <mach/isr.h>
#include <mach/pit.h>
#include <mach/io.h>
#include <sys/assert.h>

extern void selfsig(), check_events(), syscall();
extern int deliver_isr();

extern char *heap;

struct gate *idt;	/* Our IDT for VSTa */
struct segment *gdt;	/*  ...and GDT */
struct tss *tss;	/*  ...and TSS */

#ifdef KDB
/* This can be helpful sometimes while debugging */
struct trapframe *dbg_trap_frame;
ulong dbg_fault_addr;
#endif

/*
 * These wire up our IDT to point to the various handlers we
 * need for i386 traps/exceptions
 */
extern void Tdiv(), Tdebug(), Tnmi(), Tbpt(), Tovfl(), Tbound(),
	Tinstr(), T387(), Tdfault(), Tinvtss(), Tseg(), Tstack(),
	Tgenpro(), Tpgflt(), Tnpx(), Tsyscall(), Tcpsover();
struct trap_tab {
	int t_num;
	voidfun t_vec;
} trap_tab[] = {
	{T_DIV, Tdiv}, {T_DEBUG, Tdebug}, {T_NMI, Tnmi},
	{T_BPT, Tbpt}, {T_OVFL, Tovfl}, {T_BOUND, Tbound},
	{T_INSTR, Tinstr}, {T_387, T387}, {T_DFAULT, Tdfault},
	{T_INVTSS, Tinvtss}, {T_SEG, Tseg}, {T_STACK, Tstack},
	{T_GENPRO, Tgenpro}, {T_PGFLT, Tpgflt}, {T_NPX, Tnpx},
	{T_SYSCALL, Tsyscall}, {T_CPSOVER, Tcpsover},
	{0, 0}
};

/*
 * check_preempt()
 *	If appropriate, preempt current thread
 *
 * This routine will swtch() itself out as needed; just calling
 * it does the job.
 */
void
check_preempt(void)
{
	extern void timeslice();

	/*
	 * If no preemption needed, holding locks, or not running
	 * with a process, don't preempt.
	 */
	if (!do_preempt || !curthread || (cpu.pc_locks > 0)
			|| (cpu.pc_nopreempt > 0)) {
		return;
	}

	/*
	 * Use timeslice() to switch us off
	 */
	timeslice();
}

/*
 * nudge()
 *	Tell CPU to preempt
 *
 * Of course, pretty simple on uP.  Just set flag; it will be seen on
 * way back from kernel mode.
 */
void
nudge(struct percpu *c)
{
	do_preempt = 1;
}

/*
 * page_fault()
 *	Handle page faults
 *
 * This is the machine-dependent code which calls the portable vas_fault()
 * once it has figured out the faulting address and such.
 */
static void
page_fault(struct trapframe *f)
{
	ulong l;
	struct vas *vas;

	ASSERT(curthread, "page_fault: no proc");

	/*
	 * Get fault address.  Drop the high bit because the
	 * user's 0 maps to our 0x80000000, but our vas is set
	 * up in terms of his virtual addresses.
	 */
	l = get_cr2();
	if (l < 0x80000000) {
		ASSERT(f->ecs & 0x3, "trap: kernel fault");

		/*
		 * Naughty, trying to touch the kernel
		 */
		selfsig(EFAULT);
		return;
	}
	l &= ~0x80000000;
#ifdef DEBUG
	dbg_fault_addr = l;
#endif

	/*
	 * Let the portable code try to resolve it
	 */
	vas = curthread->t_proc->p_vas;
	if (vas_fault(vas, l, f->errcode & EC_WRITE)) {
		if (curthread->t_probe) {
			ASSERT(!USERMODE(f), "page_fault: probe from user");
			f->eip = (ulong)(curthread->t_probe);
		} else {
			/*
			 * Stack growth.  We try to grow it if it's
			 * a "reasonable" depth below current stack.
			 */
			if ((l < USTACKADDR) &&
					(l > (USTACKADDR-UMINSTACK))) {
				if (alloc_zfod_vaddr(vas, btop(UMAXSTACK),
						USTACKADDR-UMAXSTACK)) {
					return;
				}
			}

			/*
			 * Shoot him
			 */
			selfsig(EFAULT);
		}
	}
}

/*
 * trap()
 *	Central handling for traps
 */
void
trap(ulong place_holder)
{
	struct trapframe *f = (struct trapframe *)&place_holder;
	int kern_mode;

#ifdef KDB
	dbg_trap_frame = f;
#endif

	/*
	 * If this is first entry (from user mode), mark the place
	 * on the stack.  XXX but it's invariant, is this a waste
	 * of time?
	 */
	kern_mode = ((f->ecs & 0x3) == 0);
	if (!kern_mode) {
		ASSERT_DEBUG(curthread, "trap: user !curthread");
		ASSERT_DEBUG(curthread->t_uregs == 0, "trap: nested user");
		curthread->t_uregs = f;
	}

	/*
	 * Pick action based on trap.  System calls account for
	 * most entries, so special case them first.
	 */
	if (f->traptype == T_SYSCALL) {
		syscall(f);
	} else if (kern_mode) {
		/*
		 * We break out kernel versus user traps
		 * to encode switch tables densely and allow
		 * greater optimization.  This is the kernel
		 * table.
		 */
		switch (f->traptype) {
		case T_PGFLT:
			page_fault(f);
			break;
		case T_DIV:
			ASSERT(0, "trap: kernel divide error");
#ifdef DEBUG
		case T_DEBUG:
		case T_BPT:
			printf("trap: kernel debug\n");
			dbg_enter();
			break;
#endif
		case T_387:
		case T_NPX:
			ASSERT(0, "trap: FP used in kernel");

		default:
			printf("Trap frame in kern at 0x%x\n", f);
			ASSERT(0, "trap: bad type");
		}
	/*
	 * This is the user table of trap types (except system calls,
	 * handled before all this).
	 */
	} else switch (f->traptype) {
	case T_PGFLT:
		page_fault(f);
		break;

	case T_387:
		if (cpu.pc_flags & CPU_FP) {
			ASSERT((curthread->t_flags & T_FPU) == 0,
				"trap: T_387 but 387 enabled");
			if (curthread->t_fpu == 0) {
				curthread->t_fpu =
					MALLOC(sizeof(struct fpu), MT_FPU);
				fpu_enable((struct fpu *)0);
			} else {
				fpu_enable(curthread->t_fpu);
			}
			curthread->t_flags |= T_FPU;
			break;
		}

		/* VVV Otherwise, fall into VVV */

	case T_NPX:
		if (curthread->t_flags & T_FPU) {
			fpu_maskexcep();
		}

		/* VVV continue falling... VVV */

	case T_DIV:
	case T_OVFL:
	case T_BOUND:
		selfsig(EMATH);
		break;

	case T_DEBUG:
	case T_BPT:
#ifdef PROC_DEBUG
		f->eflags |= F_RF;	/* i386 doesn't set it */
		ASSERT_DEBUG(curthread, "trap: user debug !curthread");
		PTRACE_PENDING(curthread->t_proc, PD_BPOINT, 0);
		break;
#endif
	case T_INSTR:
		selfsig(EILL);
		break;

	case T_DFAULT:
	case T_INVTSS:
	case T_SEG:
	case T_STACK:
	case T_GENPRO:
	case T_CPSOVER:
		selfsig(EFAULT);
		break;

	default:
		printf("Trap frame at 0x%x\n", f);
		ASSERT(0, "trap: bad type");
	}
	ASSERT_DEBUG(cpu.pc_locks == 0, "trap: locks held");

	/*
	 * See if we should handle any events
	 */
	check_events();

	/*
	 * See if we should get off the CPU
	 */
	check_preempt();

	/*
	 * Clear uregs if nesting back to user
	 */
	if (!kern_mode) {
		PTRACE_PENDING(curthread->t_proc, PD_ALWAYS, 0);
		curthread->t_uregs = 0;
	}
}

/*
 * init_icu()
 *	Initial the 8259 interrupt controllers
 */
static void
init_icu(void)
{
	/* initialize 8259's */
	outportb(ICU0, 0x11);		/* Reset */
	outportb(ICU0+1, CPUIDT);	/* Vectors served */
	outportb(ICU0+1, 1 << 2);	/* Chain to ICU1 */
	outportb(ICU0+1, 1);		/* 8086 mode */
	outportb(ICU0+1, 0xff);		/* No interrupts for now */
	outportb(ICU0, 2);		/* ISR mode */

	outportb(ICU1, 0x11);		/* Reset */
	outportb(ICU1+1, CPUIDT+8);	/* Vectors served */
	outportb(ICU1+1, 2);		/* We are slave */
	outportb(ICU1+1, 1);		/* 8086 mode */
	outportb(ICU1+1, 0xff);		/* No interrupts for now */
	outportb(ICU1, 2);		/* ISR mode */
}

/*
 * init_pit()
 *	Set the main interval timer to tick at HZ times per second
 */
static void
init_pit(void)
{
	/*
	 * initialise 8254 (or 8253) channel 0.  We set the timer to
	 * generate an interrupt every time we have done enough ticks.
	 * The output format is command, LSByte, MSByte.  Note that the
	 * lowest the value of HZ can be is 19 - otherwise the
	 * calculations get screwed up,
	 */
	outportb(PIT_CTRL, CMD_SQR_WAVE);
	outportb(PIT_CH0, (PIT_TICK / HZ) & 0x00ff);
	outportb(PIT_CH0, ((PIT_TICK / HZ) & 0xff00) >> 8);
}

/*
 * setup_gdt()
 *	Switch from boot GDT to our hand-crafted one
 */
static void
setup_gdt(void)
{
	struct tss *t;
	struct segment *g, *s;
	struct linmem l;
	extern pte_t *cr3;
	extern void panic();
	extern char id_stack[];

	/*
	 * Allocate our 32-bit task and our GDT.  We will always
	 * run within the same task, just switching CR3's around.  But
	 * we need it because it tabulates stack pointers and such.
	 */
	tss = t = (struct tss *)heap;
	heap += sizeof(struct tss);
	gdt = g = (struct segment *)heap;
	heap += NGDT*sizeof(struct segment);

	/*
	 * Create 32-bit TSS
	 */
	bzero(t, sizeof(struct tss));
	t->cr3 = (ulong)cr3;
	t->eip = (ulong)panic;
	t->cs = GDT_KTEXT;
	t->ss0 = t->ds = t->es = t->ss = GDT_KDATA;
	t->esp = t->esp0 = (ulong)id_stack;

	/*
	 * Null entry--actually, zero whole thing to be safe
	 */
	bzero(g, NGDT*sizeof(struct segment));

	/*
	 * Kernel data--all 32 bits allowed, read-write
	 */
	s = &g[GDTIDX(GDT_KDATA)];
	s->seg_limit0 = 0xFFFF;
	s->seg_base0 = 0;
	s->seg_base1 = 0;
	s->seg_base2 = 0;
	s->seg_type = T_MEMRW;
	s->seg_dpl = PRIV_KERN;
	s->seg_p = 1;
	s->seg_limit1 = 0xF;
	s->seg_32 = 1;
	s->seg_gran = 1;

	/*
	 * Kernel text--low 2 gig, execute and read
	 */
	s = &g[GDTIDX(GDT_KTEXT)];
	*s = g[GDTIDX(GDT_KDATA)];
	s->seg_type = T_MEMXR;
	s->seg_limit1 = 0x7;

	/*
	 * 32-bit boot TSS descriptor
	 */
	s = &g[GDTIDX(GDT_BOOT32)];
	s->seg_limit0 = sizeof(struct tss)-1;
	s->seg_base0 = (ulong)t & 0xFFFF;
	s->seg_base1 = ((ulong)t >> 16) & 0xFF;
	s->seg_type = T_TSS;
	s->seg_dpl = PRIV_KERN;
	s->seg_p = 1;
	s->seg_limit1 = 0;
	s->seg_32 = 0;
	s->seg_gran = 0;
	s->seg_base2 = ((ulong)t >> 24) & 0xFF;

	/*
	 * 32-bit user data.  User addresses are offset 2 GB.
	 */
	s = &g[GDTIDX(GDT_UDATA)];
	s->seg_limit0 = 0xFFFF;
	s->seg_base0 = 0;
	s->seg_base1 = 0;
	s->seg_base2 = 0x80;
	s->seg_type = T_MEMRW;
	s->seg_dpl = PRIV_USER;
	s->seg_p = 1;
	s->seg_limit1 = 0x7;
	s->seg_32 = 1;
	s->seg_gran = 1;

	/*
	 * 32-bit user text
	 */
	s = &g[GDTIDX(GDT_UTEXT)];
	*s = g[GDTIDX(GDT_UDATA)];
	s->seg_type = T_MEMXR;

	/*
	 * Set GDT to our new structure
	 */
	l.l_len = (sizeof(struct segment) * NGDT)-1;
	l.l_addr = (ulong)gdt;
	lgdt(&l.l_len);

	/*
	 * Now that we have a GDT slot for our TSS, we can
	 * load the task register.
	 */
	ltr(GDT_BOOT32);
}

/*
 * set_idt()
 *	Set up an IDT slot
 */
static void
set_idt(struct gate *i, voidfun f, int typ)
{
	i->g_off0 = (ulong)f & 0xFFFF;
	i->g_sel = GDT_KTEXT;
	i->g_stkwds = 0;
	i->g_type = typ;
	i->g_dpl = PRIV_KERN;
	i->g_p = 1;
	i->g_off1 = ((ulong)f >> 16) & 0xFFFF;
}

/*
 * init_trap()
 *	Create an IDT
 */
void
init_trap(void)
{
	int x, intrlen;
	struct trap_tab *t;
	struct linmem l;
	char *p;
	extern void stray_intr(), stray_ign(), xint32(), xint33();

	/*
	 * Set up GDT first
	 */
	setup_gdt();

	/*
	 * Set ICUs to known state
	 */
	init_icu();

	/*
	 * Set the interval timer to the correct tick speed
	 */
	init_pit();

	/*
	 * Carve out an IDT table for all possible 256 sources
	 */
	idt = (struct gate *)heap;
	heap += (sizeof(struct gate) * NIDT);

	/*
	 * Set all trap entries to initially log stray events
	 */
	bzero(idt, sizeof(struct gate) * NIDT);
	for (x = 0; x < CPUIDT; ++x) {
		set_idt(&idt[x], stray_intr, T_TRAP);
	}

	/*
	 * Wire all interrupts to a vector which will push their
	 * interrupt number and call our common C code.
	 */
	p = (char *)xint32;
	intrlen = (char *)xint33 - (char *)xint32;
	for (x = CPUIDT; x < NIDT; ++x) {
		set_idt(&idt[x],
			(voidfun)(p + (x - CPUIDT)*intrlen),
			T_INTR);
	}

	/*
	 * Map interrupt 7 to a fast ingore.  I get bursts of these
	 * even when IRQ 7 is masked from the PIC.  It only happens
	 * when I enable the slave PIC, so it smells like hardware
	 * weirdness.
	 */
	set_idt(&idt[CPUIDT+7], stray_ign, T_INTR);

	/*
	 * Hook up the traps we understand
	 */
	for (t = trap_tab; t->t_vec; ++t) {
		set_idt(&idt[t->t_num], t->t_vec, T_TRAP);
	}

	/*
	 * Users can make system calls with "int $T_SYSCALL"
	 */
	idt[T_SYSCALL].g_dpl = PRIV_USER;

	/*
	 * Load the IDT into hardware
	 */
	l.l_len = (sizeof(struct gate) * NIDT)-1;
	l.l_addr = (ulong)idt;
	lidt(&l.l_len);
}

/*
 * sendev()
 *	Do machinery of event delivery
 *
 * "thread" is passed, but must be the current process.  I think this
 * is a little more efficient than using curthread.  I'm also thinking
 * that it might be worth it to make this routine work across threads.
 * Maybe.
 */
void
sendev(struct thread *t, char *ev)
{
	struct evframe e;
	struct trapframe *f;
	struct proc *p;
	extern int do_exit();

	ASSERT(t->t_uregs, "sendev: no user frame");
	f = t->t_uregs;

	/*
	 * If no handler or KILL, process dies
	 */
	p = t->t_proc;
	if (!p->p_handler || !strcmp(ev, EKILL)) {
		strcpy(p->p_event, ev);
		do_exit(_W_EV);
	}

	/*
	 * Build event frame
	 */
	e.ev_prevsp = f->esp;
	e.ev_previp = f->eip;
	strcpy(e.ev_event, ev);

	/*
	 * Try and place it on the stack
	 */
	if (copyout(f->esp - sizeof(e), &e, sizeof(e))) {
#ifdef DEBUG
		printf("Stack overflow pid %ld/%ld sp 0x%x\n",
			p->p_pid, t->t_pid, f->esp);
		dbg_enter();
#endif
		do_exit(1);
	}

	/*
	 * Update user's registers to reflect this nesting
	 */
	f->esp -= sizeof(e);
	f->eip = (ulong)(p->p_handler);
}

/*
 * interrupt()
 *	Common code for all CPU interrupts
 */
void
interrupt(ulong place_holder)
{
	struct trapframe *f = (struct trapframe *)&place_holder;
	int isr = f->traptype;

	/*
	 * Sanity check and fold into range 0..MAX_IRQ-1.  Enable
	 * further interrupts from the ICU--interrupts are still
	 * masked on-chip.
	 */
	ASSERT(isr >= T_EXTERN, "interrupt: stray low");
	isr -= T_EXTERN;
	ASSERT(isr < MAX_IRQ, "interrupt: stray high");
	EOI();

	/*
	 * Our only hard-wired interrupt handler; the clock
	 */
	if (isr == 0) {
		extern void hardclock();

		hardclock(f);
		goto out;
	}

	/*
	 * Let processes registered for an IRQ get them
	 */
	if (deliver_isr(isr)) {
		goto out;
	}

	/*
	 * Otherwise bomb on stray interrupt
	 */
	ASSERT(0, "interrupt: stray");

	/*
	 * Check for preemption and events if we pushed in from user mode.
	 * When ready for kernel preemption, move check_preempt() to before
	 * the "if" statement.
	 */
out:
	if ((f->ecs & 0x3) == PRIV_USER) {
		struct thread *t = curthread;

		sti();
		if (EVENT(t)) {
			t->t_uregs = f;
			check_events();
			t->t_uregs = 0;
		}
		check_preempt();
	}
}
@


1.22
log
@Optimize common case of system call entry to kernel
@
text
@a183 5
	} else {
		/*
		 * Make trap type distinct for kernel
		 */
		f->traptype |= T_KERNEL;
d192 32
a224 1
	case T_PGFLT|T_KERNEL:
a228 3
	case T_DIV|T_KERNEL:
		ASSERT(0, "trap: kernel divide error");

a258 7
#ifdef DEBUG
	case T_DEBUG|T_KERNEL:
	case T_BPT|T_KERNEL:
		printf("trap: kernel debug\n");
		dbg_enter();
		break;
#endif
a269 6

	case T_387|T_KERNEL:
	case T_NPX|T_KERNEL:
		ASSERT(0, "trap: FP used in kernel");

	/* case T_DFAULT|T_KERNEL: XXX stack red zones? */
@


1.21
log
@General cleanup
@
text
@d192 2
a193 1
	 * Pick action based on trap
d195 3
a197 1
	switch (f->traptype) {
a267 4
		break;

	case T_SYSCALL:
		syscall(f);
@


1.20
log
@Add FPU support
@
text
@d90 1
d164 1
d173 1
a346 1
	struct gate *i;
d349 1
a349 1
	extern void xsyscall(), panic();
@


1.19
log
@Remove debug, use USERMODE instead of hard-coded value
@
text
@d10 1
d18 1
d42 1
a42 1
	Tgenpro(), Tpgflt(), Tnpx(), Tsyscall();
d52 1
a52 1
	{T_SYSCALL, Tsyscall},
a106 1
	extern ulong get_cr2();
d200 24
a226 2
	case T_387:
	case T_NPX:
d233 1
a233 1
		printf("Kernel debug trap\n");
d251 1
a251 1
		ASSERT(0, "387 used in kernel");
d260 1
@


1.18
log
@Stack growth
@
text
@d135 1
a135 5
#ifdef DEBUG
			printf("cpfail\n"); dbg_enter();
#endif
			ASSERT((f->ecs & 3) == PRIV_KERN,
				"page_fault: probe from user");
@


1.17
log
@Fix sched node memory leak; add explicit preemption inhibit
@
text
@d104 1
d132 2
a133 2
	if (vas_fault(curthread->t_proc->p_vas, l,
			f->errcode & EC_WRITE)) {
d142 15
@


1.16
log
@Get rid of trap to kdb on each killed process
@
text
@d70 2
a71 1
	if (!do_preempt || !curthread || (cpu.pc_locks > 0)) {
@


1.15
log
@Pass trap frame pointer to hardclock()
@
text
@a534 5
#ifdef DEBUG
		printf("tid %d dies on unhandled event: %s\n",
			t->t_pid, ev);
		dbg_enter();
#endif
@


1.14
log
@Allow CPU-bound threads to see events, to wit, when they enter
the kernel on the clock tick.
@
text
@d596 1
a596 1
		hardclock();
@


1.13
log
@Add programming of timer to get a rational clock speed
@
text
@d19 1
a19 1
extern void selfsig();
a152 1
	extern void check_events(), syscall();
d613 3
a615 3
	 * Check for preemption if we pushed in from user mode
	 * XXX should allow preemption from kernel too.  "Should"
	 * work, but I don't want to chase too many bugs at once!
d619 2
d622 5
@


1.12
log
@RF not set on debug register break.  Set it manually, so you
can continue after such a breakpoint.
@
text
@d16 1
d281 19
d451 5
@


1.11
log
@Add ptrace hooks
@
text
@d204 1
@


1.10
log
@Reorder check_preempt/events.  We shouldn't preempt if we're
going to die first.
@
text
@d5 1
a7 1
#include <sys/proc.h>
a9 1
#include <mach/machreg.h>
d203 5
d250 1
@


1.9
log
@Change exit() -> do_exit() to make gcc happy
@
text
@d133 3
a135 1
			printf("cpfail\n");
d233 1
a233 1
	 * See if we should get off the CPU
d235 1
a235 1
	check_preempt();
d238 1
a238 1
	 * See if we should handle any events
d240 1
a240 1
	check_events();
@


1.8
log
@Fix struct alignment (use explicit sub-parts)
@
text
@d493 1
a493 1
	extern void exit();
d509 1
a509 1
		exit(_W_EV);
d528 1
a528 1
		exit(1);
@


1.7
log
@Implement KDB
@
text
@d318 1
d339 2
a340 1
	s->seg_base0 = (ulong)t & 0xFFFFFF;
d347 1
a347 1
	s->seg_base1 = ((ulong)t >> 24) & 0xFF;
d355 2
a356 1
	s->seg_base1 = 0x80;
@


1.6
log
@Add machinery for telling why a proc died
@
text
@d28 1
a28 1
#ifdef DEBUG
d153 1
a153 1
#ifdef DEBUG
@


1.5
log
@Enable timeslice preemption.  Also add a debug hook
for now to watch processes die unusual deaths.
@
text
@d8 2
a16 1
#include <sys/fs.h>
d505 2
a506 1
		exit(1);
@


1.4
log
@Workaround for motherboard weirdness
@
text
@d499 5
d560 1
a560 1
		return;
d567 1
a567 1
		return;
d574 11
@


1.3
log
@Add assertion to catch stray locks
@
text
@d408 1
a408 1
	extern void stray_intr(), xint32(), xint33();
d445 8
@


1.2
log
@Move preemption flag to percpu struct
@
text
@d227 1
@


1.1
log
@Initial revision
@
text
@a22 1
int do_preempt = 0;	/* Flag preempt requested */
@
