head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.4
date	93.08.24.00.41.03;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.08.15.09.54;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.03.20.14.17;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.50.54;	author vandys;	state Exp;
branches;
next	;


desc
@Simple wrapper to call some init functions
@


1.4
log
@Add kdb support for console, plus hooks so you can still do serial
if needed.
@
text
@/*
 * main.c
 *	Initial C code run during bootup
 */
#include <sys/mutex.h>

extern void init_machdep(), init_page(), init_qio(), init_sched(),
	init_proc(), init_swap(), swtch(), init_malloc(), init_msg();
extern void init_wire(), start_clock(), init_cons();
#ifdef DEBUG
extern void init_debug();
#endif

extern lock_t runq_lock;

int upyet = 0;	/* Set to true once basic stuff initialized */

main(void)
{
	init_machdep();
	init_page();
	init_malloc();
	init_cons();
#ifdef DEBUG
	init_debug();
#endif
	init_qio();
	init_sched();
	init_proc();
	init_msg();
	init_swap();
	init_wire();
	start_clock();

	/*
	 * Flag that we're up.  swtch() assumes runq_lock is held,
	 * so take it and fall into the scheduler.
	 */
	upyet = 1;
	(void)p_lock(&runq_lock, SPLHI);
	swtch();	/* Assumes curthread is 0 currently */
}
@


1.3
log
@Add call to start real-time clock
@
text
@d9 1
a9 1
extern void init_wire(), start_clock();
d23 1
@


1.2
log
@Add physical I/O support
@
text
@d9 1
a9 1
extern void init_wire();
d32 1
@


1.1
log
@Initial revision
@
text
@d9 1
d31 1
@
