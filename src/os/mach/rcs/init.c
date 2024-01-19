head	1.8;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.8
date	94.12.23.05.12.28;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.12.21.05.27.06;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.10.05.17.57.46;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.03.04.02.01.11;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.44.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.06.30.02.28.14;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.14.14.23;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.05.21;	author vandys;	state Exp;
branches;
next	;


desc
@Low-level setup during boot
@


1.8
log
@Remove global from here; doesn't really fit
@
text
@/*
 * init.c
 *	Set up initial environment for VSTa on i386
 */
#include <mach/pte.h>
#include <sys/percpu.h>
#include <mach/aout.h>
#include <sys/assert.h>
#include <sys/param.h>
#include <mach/vm.h>
#include <sys/boot.h>
#include <sys/mutex.h>
#include <rmap.h>
#include <mach/pte.h>
#include <sys/vm.h>
#include <mach/kbd.h>
#include <mach/io.h>
#include <mach/machreg.h>
#include <std.h>

#define K (1024)

/* a.out header is part of first page of text */
#define KERN_AOUT_HDR ((struct aout *)0x1000)

extern void init_trap();

char *mem_map_base;	/* Base of P->V mapping area */
char *heap,		/* Physical heap used during bootup */
	*heapstart;	/* Where it starts */

struct percpu cpu;	/* Only one for i386 uP */
uint ncpu = 1;
struct percpu *nextcpu = &cpu;

struct boot_task *boot_tasks;
uint nboot_task = 0;

struct rmap *vmap;	/* Map for virtual memory */

uint freel1pt;		/* First free slot after bootup */

/*
 * The two memory ranges supported under i386/ISA.  The size of
 * extended memory is handed to us from the boot loader--who
 * gets it from the NVRAM config data.
 */
struct memseg memsegs[2];
int nmemsegs = 2;
pte_t *cr3;
int bootpgs;		/* Pages of memory available at boot */

/*
 * init_mem()
 *	Set up to use all of memory
 */
static void
init_mem(void)
{
	/*
	 * Enable the high address stuff.  It hangs off the
	 * keyboard controller.  Brilliant.
	 */
	while (inportb(KEYBD_STATUS) & KEYBD_BUSY)
		;
	outportb(KEYBD_STATUS, KEYBD_WRITE);
	while (inportb(KEYBD_STATUS) & KEYBD_BUSY)
		;
	outportb(KEYBD_DATA, KEYBD_ENAB20);
}

/*
 * init_machdep()
 *	Initialize machine-dependent memory stuff
 */
void
init_machdep(void)
{
	pte_t *pt;
	struct aout *a;
	struct boot_task *b;
	int have_fpu, x, y, pgs;
	ulong cr0;
	extern uint free_pfn, size_base, size_ext, boot_pfn;

	/*
	 * Probe FPU
	 */
	fpu_enable(0);
	have_fpu = fpu_detected();
	fpu_disable(0);

	/*
	 * Initialize our single "per CPU" data structure
	 */
	bzero(&cpu, sizeof(cpu));
	cpu.pc_flags = CPU_UP|CPU_BOOT;
	if (have_fpu) {
		cpu.pc_flags |= CPU_FP;
	}
	cpu.pc_next = &cpu;

	/*
	 * Set up CR0.  Clear "task switched", set emulation.
	 */
	cr0 = get_cr0();
	cr0 &= ~(CR0_TS);
	cr0 |=  (CR0_MP | CR0_NE | CR0_EM);
	set_cr0(cr0);

	/*
	 * Set up memory control
	 */
	init_mem();

	/*
	 * Apply sanity checks to values our boot loader provided.
	 */
	ASSERT(size_base == 640*K, "need 640K base mem");
	ASSERT(size_ext >= K*K, "need 1M extended mem");
	memsegs[0].m_base = 0;
	memsegs[0].m_len = 640*K;
	memsegs[1].m_base = (void *)(K*K);
	memsegs[1].m_len = size_ext;
	if (size_ext > 15*K*K) {
		memsegs[1].m_len = 15*K*K;
	}

	/*
	 * Set up our heap.  Amusingly, because data starts at 4 Mb
	 * but text is 1:1, our heap lies between text and data.
	 * Because only the first 640K is contiguous, "heap" can't be
	 * used for any really large data structures.  This *IS* a
	 * microkernel....
	 *
	 * N.B., this is the bootup heap.  Once we're up the malloc()
	 * interface can use any memory on the system.
	 */
	heapstart = heap = (char *)ptob(free_pfn);

	/*
	 * Starting at "end" we have one or more task images.  We
	 * must manually construct processes for them so that they
	 * will be scheduled when we start running processes.  This
	 * technique is used to avoid having to imbed boot drivers/
	 * filesystems/etc. into the microkernel.  We're not ready
	 * to do full task creation, but now is a good time to
	 * tabulate them.
	 */
	a = (struct aout *)ptob(boot_pfn);
	b = boot_tasks = (struct boot_task *)heap;
	while ((char *)a < heapstart) {
		/*
		 * Convert from a.out-ese into a more generic
		 * representation.
		 */
		b->b_pc = a->a_entry;
		b->b_textaddr = (char *)NBPG;
		b->b_text = btorp(a->a_text + sizeof(struct aout));
		b->b_dataaddr = (void *)(NBPG*K);
		b->b_data = btorp(a->a_data + a->a_bss);
		b->b_pfn = btop(a);

		/*
		 * Advance and iterate until we run into the first
		 * free page (and thus, the last boot task)
		 */
		nboot_task += 1;
		++b;
		a = (struct aout *)((char *)a +
			(sizeof(struct aout) +
			 a->a_text + a->a_data + a->a_bss));
		a = (struct aout *)roundup(a, NBPG);
	}
	heap = (char *)b;

	/*
	 * Get a resource map for our utility virtual pool.  The PTEs
	 * for the virtual space will be set up shortly.
	 */
	vmap = (struct rmap *)heap;
	heap += sizeof(struct rmap)*VMAPSIZE;
	rmap_init(vmap, VMAPSIZE);
	rmap_free(vmap, btop(L1PT_UTIL*BYTES_L1PT), NPTPG);

	/*
	 * Get our "real" root page table.  We've been using the one
	 * from the boot loader in high memory, but it will be more
	 * convenient to use a one in boot memory, safely out of reach
	 * of the memory allocator.
	 *
	 * Note that the mappings for text and data, unlike the one
	 * provided by the boot environment, only map the actual
	 * contents of the text and data segments.
	 */
	heap = (char *)roundup(heap, NBPG);
	cr3 = (pte_t *)heap; heap += NBPG;
	bzero(cr3, NBPG);

	/*
	 * Build entry 0--1:1 map for text.  Note we start at index 1
	 * to leave an invalid page at vaddr 0--this catches null
	 * pointer accesses, usually.  We still map low memory 1:1,
	 * since we continue to use the heap until the page pool can
	 * be initialized.
	 */
	pt = (pte_t *)heap; heap += NBPG;
	cr3[L1PT_TEXT] = (ulong)pt | PT_V|PT_W;
	bzero(pt, NBPG);
	for (x = 1; x < NPTPG; ++x) {
		pt[x] = (x << PT_PFNSHIFT) | PT_V|PT_W;
	}

	/*
	 * Build entry 1--map of data.  Data always starts at 4 Mb
	 * virtual, but its actual contents is merely the next physical
	 * page after the end of text.  This is calculated as one
	 * page for the invalid NULL page, the size of the a.out header
	 * (which resides just before text), and the size of text
	 * itself.
	 */
	pt = (pte_t *)heap; heap += NBPG;
	cr3[L1PT_DATA] = (ulong)pt | PT_V|PT_W;
	bzero(pt, NBPG);
	x = btorp(NBPG + sizeof(struct aout) + KERN_AOUT_HDR->a_text);
	pgs = btorp(KERN_AOUT_HDR->a_data + KERN_AOUT_HDR->a_bss);
	for (y = 0; y < pgs; ++x,++y) {
		pt[y] = (x << PT_PFNSHIFT) | PT_V|PT_W;
	}

	/*
	 * Entry 2--recursive map of page tables
	 */
	cr3[L1PT_CR3] = (ulong)cr3 | PT_V|PT_W;

	/*
	 * Entry 3--utility
	 */
	pt = (pte_t *)heap; heap += NBPG;
	cr3[L1PT_UTIL] = (ulong)pt | PT_V|PT_W;
	bzero(pt, NBPG);

	/*
	 * Set up a 1:1 mapping of memory now that we know
	 * its size.  We will use the next free L1PT entry,
	 * and use as many as it takes.  Note that the base
	 * 640K counts as 1 M for purposes of the global
	 * 1:1 map.
	 */
	pt = (pte_t *)heap;
	y = L1PT_FREE;
	bootpgs = pgs =
		btop(memsegs[1].m_base) + btop(memsegs[1].m_len);
	for (x = 0; x < pgs; ++x) {
		if ((x % NPTPG) == 0) {
			cr3[y++] = (((ulong)(&pt[x])) & PT_PFN) |
				PT_V|PT_W;
			heap += NBPG;
		}
		pt[x] = (x << PT_PFNSHIFT) | PT_V|PT_W;
	}
	mem_map_base = (char *)(L1PT_FREE * BYTES_L1PT);

	/*
	 * Switch to our own PTEs
	 */
	set_cr3((ulong)cr3);

	/*
	 * Leave index of next free slot in kernel part of L1PTEs.
	 */
	freel1pt = y;

	/*
	 * Set up interrupt system
	 */
	init_trap();
}
@


1.7
log
@Add counter for number of SRUN processes
@
text
@a37 1
uint num_run = 0;	/* Number of currently runnable threads */
@


1.6
log
@Add FPU support
@
text
@d38 1
d126 3
@


1.5
log
@Fix -Wall warnings
@
text
@d18 1
d82 2
a83 1
	int x, y, pgs;
a84 1
	extern void set_cr3(pte_t *);
d87 7
d98 3
d104 8
d264 1
a264 1
	set_cr3(cr3);
@


1.4
log
@Source reorg
@
text
@d17 2
d83 1
@


1.3
log
@Don't trust _etext, _end any more--they've changed in GCC 2.X.
Besides, I had a bug because I walked from etext to end as
the size of data--but end is up in the 4Mb+ area!
@
text
@d13 1
a13 1
#include <lib/rmap.h>
d16 1
a16 1
#include <kbd/kbd.h>
@


1.2
log
@Zero all fields of CPU
@
text
@d20 3
a24 3
extern char end[],	/* Where heap starts */
	etext[];	/* End of text */

d190 6
a195 1
	 * Build entry 1--map of data
d200 3
a202 2
	x = btorp(etext);
	for (y = 0; x < btorp(end); ++x,++y) {
@


1.1
log
@Initial revision
@
text
@d85 1
a85 3
	cpu.pc_thread = 0;
	cpu.pc_num = 0;
	cpu.pc_locks = 0;
@
