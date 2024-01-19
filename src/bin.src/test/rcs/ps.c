head	1.2;
access;
symbols;
locks; strict;
comment	@ * @;


1.2
date	94.07.10.17.52.01;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.07.10.17.42.41;	author vandys;	state Exp;
branches;
next	;


desc
@Process Status display program
@


1.2
log
@Add header
@
text
@/*
 * ps.c
 *	Poor man's ps(1)
 */
#include <sys/pstat.h>
#include <std.h>

/*
 * Container for a pstat entry so we can keep them
 * in sorted order.
 */
struct plist {
	struct pstat pl_pstat;
	struct plist *pl_next;
};

/*
 * statename()
 *	Convert process state to (roughly) string name of CPU state
 *
 * Doomed to failure, since one can have multiple threads in multiple
 * states, but works for "classic" single-threaded procs.
 */
static char *
statename(struct pstat *p)
{
	if (p->ps_nonproc > 0) {
		return("ONPROC");
	} else if (p->ps_nrun > 0) {
		return("RUN   ");
	} else {
		return("SLP   ");
	}
}

/*
 * printps()
 *	Dump out a process entry
 */
static void
printps(struct pstat *p)
{
	printf("%6d %-8s %s %7d %d/%d\n", p->ps_pid, p->ps_cmd,
		statename(p), p->ps_nthread,
		p->ps_usrcpu, p->ps_syscpu);
}

main()
{
	struct pstat ps;
	pid_t startpid;
	int havefirst = 0;
	struct plist *phead = 0, *p, **pp, *pnew;

	for (;;) {
		/*
		 * Get next slot
		 */
		if (pstat(&ps, 1, sizeof(ps)) != 1) {
			break;
		}

		/*
		 * Handle end case, recording first we see and
		 * terminating when we come back around to it.
		 */
		if (!havefirst) {
			startpid = ps.ps_pid;
			havefirst = 1;
		} else {
			if (ps.ps_pid == startpid) {
				break;
			}
		}

		/*
		 * Add pstat entry in its place
		 */
		pnew = malloc(sizeof(struct plist));
		if (pnew == 0) {
			perror("ps: proc list");
			exit(1);
		}
		pnew->pl_pstat = ps;
		pp = &phead;
		for (p = phead; p; p = p->pl_next) {
			if (p->pl_pstat.ps_pid > ps.ps_pid) {
				break;
			}
			pp = &p->pl_next;
		}
		pnew->pl_next = *pp;
		*pp = pnew;
	}

	/*
	 * Print them
	 */
	printf("PID    CMD      STATE  NTHREAD USR/SYS\n");
	for (p = phead; p; p = p->pl_next) {
		printps(&p->pl_pstat);
	}
	return(0);
}
@


1.1
log
@Initial revision
@
text
@d28 1
a28 1
		return("RUNNING ");
d30 1
a30 1
		return("RUNNABLE");
d32 1
a32 1
		return("SLEEPING");
d43 1
a43 1
	printf("%6d %-8s %s %2d %d/%d\n", p->ps_pid, p->ps_cmd,
d99 1
@
