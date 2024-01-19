head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.01.29.16.08.20;	author vandys;	state Exp;
branches;
next	;


desc
@Machine-dependent VM stuff
@


1.1
log
@Initial revision
@
text
@/*
 * vm.c
 *	Machine-dependent support procedures
 */
#include <sys/vm.h>
#include <mach/vm.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <mach/pte.h>
#include <sys/assert.h>

/*
 * Precalculated address range for utility mapping area
 */
#define MAPLOW ((void *)(BYTES_L1PT*L1PT_UTIL))
#define MAPHIGH ((void *)(BYTES_L1PT*L1PT_UTIL+BYTES_L1PT))

/*
 * A static L1PT with only root entries.  Nice because it'll work
 * even when you're running on the idle stack.
 */
extern pte_t *cr3;

/*
 * Place where, because we recursively remap our root, all the
 * L2 page tables are visible.
 */
static pte_t *l2ptmap = (pte_t *)(L1PT_CR3*BYTES_L1PT);

/*
 * vtop()
 *	Return physical address for kernel virtual one
 *
 * Only works for kernel addresses, since it uses information from
 * a root page table which only uses the L2PTEs for root mappings.
 */
void *
vtop(void *vaddr)
{
	pte_t *pt;

	/*
	 * Point to base of L1PTEs
	 */
	pt = cr3;

	/*
	 * See if L1 is valid
	 */
	pt += L1IDX(vaddr);
	ASSERT(*pt & PT_V, "vtop: invalid L1");

	/*
	 * If it is, walk down to second level
	 */
	pt = l2ptmap + ((ulong)vaddr >> PT_PFNSHIFT);
	ASSERT(*pt & PT_V, "vtop: invalid L2");

	/*
	 * Construct physical addr by preserving old offset, but
	 * swapping in new pfn.
	 */
	return (void *)((*pt & PT_PFN) | ((ulong)vaddr & ~PT_PFN));
}

/*
 * kern_addtrans()
 *	Add a kernel translation
 */
void
kern_addtrans(void *vaddr, uint pfn)
{
	pte_t *pt;

	ASSERT_DEBUG((vaddr >= MAPLOW) && (vaddr < MAPHIGH),
		"kern_addtrans: bad vaddr");
	pt = l2ptmap + ((ulong)vaddr >> PT_PFNSHIFT);
	*pt = (pfn << PT_PFNSHIFT) | PT_V|PT_W;
}

/*
 * kern_deletetrans()
 *	Delete a kernel translation
 */
void
kern_deletetrans(void *vaddr, uint pfn)
{
	pte_t *pt;

	ASSERT_DEBUG((vaddr >= MAPLOW) && (vaddr < MAPHIGH),
		"kern_deletetrans: bad vaddr");
	pt = l2ptmap + ((ulong)vaddr >> PT_PFNSHIFT);
	*pt = 0;
#ifdef DEBUG
	flush_tlb();
#endif
}
@
