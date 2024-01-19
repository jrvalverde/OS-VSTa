head	1.5;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	95.01.27.17.09.55;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	95.01.10.05.16.44;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.43.08;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.30.01.12.56;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.26.23.34.00;	author vandys;	state Exp;
branches;
next	;


desc
@Interface to fire up swap daemon
@


1.5
log
@Convert to use of path_open(), also allow -n to indicate
pure page reclamation (so you can do VM page stealing without
swap)
@
text
@/*
 * swapd.c
 *	Launch all the stuff needed for the swapping system
 */
#include <sys/types.h>
#include <sys/ports.h>
#include <sys/fs.h>
#include <sys/swap.h>

#define NQIO (4)		/* # parallel qio's */

static void
runqio(void)
{
	run_qio();
	perror("swapd run_qio failed");
	exit(1);
}

main(argc, argv)
	int argc;
	char **argv;
{
	/*
	 * Set up for swapping unless -n; for -n, we are only
	 * launching a thread to do clean page stealing (for
	 * systems without a swap partition).
	 */
	if ((argc < 2) || strcmp(argv[1], "-n")) {
		port_t p;
		int x;

		/*
		 * Connect to swap daemon
		 */
		p = msg_connect(PORT_SWAP, ACC_READ|ACC_WRITE);
		if (p < 0) {
			perror("swapd can't access daemon");
			exit(1);
		}

		/*
		 * If there's an initial swap device, hand it to the
		 * swapper now.
		 */
		if (argc > 1) {
			struct swapadd s;
			port_name n;
			struct msg m;

			strcpy(s.s_path, argv[1]);
			s.s_off = 0;
			s.s_len = 0;

			m.m_op = SWAP_ADD;
			m.m_buf = &s;
			m.m_buflen = sizeof(s);
			m.m_nseg = 1;
			m.m_arg = m.m_arg1 = 0;
			if (msg_send(p, &m) < 0) {
				perror("swapd initial swap");
				exit(1);
			}
		}

		/*
		 * Hand off daemon to kernel
		 */
		if (set_swapdev(p) < 0) {
			perror("swapd can't configure swap port");
			exit(1);
		}
		for (x = 0; x < NQIO; ++x) {
			if (tfork(runqio) < 0) {
				perror("swapd qio fork");
			}
		}
	}

	/*
	 * Launch thread dedicated to scanning memory
	 */
	pageout();
	perror("swapd can't start pageout");

	exit(1);
}
@


1.4
log
@Convert swapd to path_open format for swap devices
@
text
@a23 3
	port_t p;
	int x;

d25 3
a27 1
	 * Connect to swap daemon
d29 36
a64 5
	p = msg_connect(PORT_SWAP, ACC_READ|ACC_WRITE);
	if (p < 0) {
		perror("swapd can't access daemon");
		exit(1);
	}
d66 5
a70 20
	/*
	 * If there's an initial swap device, hand it to the
	 * swapper now.
	 */
	if (argc > 1) {
		struct swapadd s;
		port_name n;
		struct msg m;

		strcpy(s.s_path, argv[1]);
		s.s_off = 0;
		s.s_len = 0;

		m.m_op = SWAP_ADD;
		m.m_buf = &s;
		m.m_buflen = sizeof(s);
		m.m_nseg = 1;
		m.m_arg = m.m_arg1 = 0;
		if (msg_send(p, &m) < 0) {
			perror("swapd initial swap");
d73 5
d81 1
a81 1
	 * Hand off daemon to kernel
a82 9
	if (set_swapdev(p) < 0) {
		perror("swapd can't configure swap port");
		exit(1);
	}
	for (x = 0; x < NQIO; ++x) {
		if (tfork(runqio) < 0) {
			perror("swapd qio fork");
		}
	}
d85 1
@


1.3
log
@Source reorg
@
text
@d45 1
a45 2
		n = namer_find(argv[1]);
		s.s_port = n;
d48 1
a48 5
		if (argc > 2) {
			strcpy(s.s_path, argv[2]);
		} else {
			s.s_path[0] = '\0';
		}
@


1.2
log
@Add call to pageout() to initiate page stealing thread
@
text
@d8 1
a8 1
#include <swap/swap.h>
@


1.1
log
@Initial revision
@
text
@d72 1
a72 1
	for (x = 1; x < NQIO; ++x) {
d77 3
a79 1
	runqio();
@
