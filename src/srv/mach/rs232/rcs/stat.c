head	1.3;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.10.06.00.01.12;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.05.30.21.26.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.11.25.04.42.37;	author vandys;	state Exp;
branches;
next	;


desc
@Stat support
@


1.3
log
@Update server; lotsa new devices supported, lots new options
@
text
@/*
 * stat.c
 *	Do the stat function
 */
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include "rs232.h"
#include "fifo.h"

extern char *perm_print();

extern struct prot rs232_prot;
extern uint accgen;
extern int irq, iobase;
extern int baud, databits, stopbits, parity;
extern int rx_fifo_threshold, tx_fifo_threshold;
extern uchar dsr, dtr, cts, rts, dcd, ri;
extern struct fifo *inbuf, *outbuf;
extern int uart;
extern port_name rs232port_name;
extern char uart_names[][RS232_UARTNAMEMAX];

static char parity_names[5][5] = {"none", "even", "odd", "zero", "one"};

/*
 * rs232_stat()
 *	Do stat requests
 */
void
rs232_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];

	if (!(f->f_flags & ACC_READ)) {
		msg_err(m->m_sender, EPERM);
		return;
	}
	rs232_getinsigs();
	sprintf(buf,
		"size=0\ntype=c\nowner=0\ninode=0\ngen=%d\n%s" \
		"dev=%d\nuart=%s\nbaseio=0x%x\nirq=%d\n" \
		"rxfifothr=%d\ntxfifothr=%d\n" \
		"baud=%d\ndatabits=%d\nstopbits=%s\nparity=%s\n" \
		"dsr=%d\ndtr=%d\ncts=%d\nrts=%d\ndcd=%d\nri=%d\n" \
		"inbuf=%d\noutbuf=%d\n",
		accgen, perm_print(&rs232_prot),
		rs232port_name, uart_names[uart],
		iobase, irq,
		rx_fifo_threshold, tx_fifo_threshold,
		baud, databits,
		(stopbits == 1 ? "1" : (databits == 5 ? "1.5" : "2")),
		parity_names[parity],
		dsr, dtr, cts, rts, dcd, ri,
		inbuf->f_cnt, outbuf->f_cnt);
	m->m_buf = buf;
	m->m_arg = m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * rs232_wstat()
 *	Allow writing of supported stat messages - rather a lot of them :-)
 */
void
rs232_wstat(struct msg *m, struct file *f)
{
	char *field, *val;

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, &rs232_prot, f->f_flags, &field, &val) == 0)
		return;

	/*
	 * Process each kind of field we can write
	 */
	if (!strcmp(field, "rts")) {
		/*
		 * Set the RTS state - default is to enabled
		 */
		int newrts;
		
		newrts = val ? atoi(val) : 1;
		rs232_setrts(newrts);
	} else if (!strcmp(field, "dtr")) {
		/*
		 * Set the DTR state - default is to enabled
		 */
		int newdtr;
		
		newdtr = val ? atoi(val) : 1;
		rs232_setdtr(newdtr);
	} else if (!strcmp(field, "gen")) {
		/*
		 * Set access-generation field
		 */
		if (val) {
			accgen = atoi(val);
		} else {
			accgen += 1;
		}
		f->f_gen = accgen;
	} else if (!strcmp(field, "baud")) {
		/*
		 * Set the connection baud rate
		 */
		int brate;

		brate = val ? atoi(val) : 9600;
		rs232_baud(brate);
	} else if (!strcmp(field, "databits")) {
		/*
		 * Set the number of data bits
		 */
		int dbits;

		dbits = val ? atoi(val) : 8;
		if (dbits < 5 || dbits > 8) {
			/*
			 * Not a value we support - fail!
			 */
			msg_err(m->m_sender, EINVAL);
			return;
		}
		rs232_databits(dbits);
	} else if (!strcmp(field, "stopbits")) {
		/*
		 * Set the number of stop bits
		 */
		int sbits;

		if (!strcmp(val, "2") || !strcmp(val, "1.5")) {
			sbits = 2;
		} else if (!strcmp(val, "1")) {
			sbits = 1;
		} else {
			/*
			 * Not a value we support - fail!
			 */
			msg_err(m->m_sender, EINVAL);
			return;
		}
		rs232_stopbits(sbits);
	} else if (!strcmp(field, "parity")) {
		/*
		 * Set the type of parity to be used
		 */
		int i, ptype = -1;

		for (i = 0; i < 5; i++) {
			if (!strcmp(val, parity_names[i])) {
				ptype = i;
				break;
			}
		}
		if (ptype == -1) {
			/*
			 * Not a value we support - fail!
			 */
			msg_err(m->m_sender, EINVAL);
			return;
		}
		rs232_parity(ptype);
	} else if (!strcmp(field, "rxfifothr")) {
		/*
		 * Set the UART receiver FIFO threshold
		 */
		int t;
		
		t = val ? atoi(val) : 0;
		if (rs232_setrxfifo(t)) {
			/*
			 * We don't support that value on this UART
			 */
			msg_err(m->m_sender, EINVAL);
			return;
		}
	} else {
		/*
		 * Not a field we support - fail!
		 */
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Return success
	 */
	m->m_buflen = m->m_nseg = m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}
@


1.2
log
@Updates for support of RS-232 line control, syslog
@
text
@d17 1
d20 3
d42 2
a43 1
		"baseio=0x%x\nirq=%d\n" \
d48 1
d50 1
d168 14
@


1.1
log
@Initial revision
@
text
@d9 1
d15 4
d20 2
d24 1
a24 1
 *	Do stat
d35 1
d37 12
a48 2
	 "size=0\ntype=c\nowner=0\ninode=0\ngen=%d\n", accgen);
	strcat(buf, perm_print(&rs232_prot));
d58 1
a58 1
 *	Allow writing of supported stat messages
d74 9
a82 1
	if (!strcmp(field, "gen")) {
d84 8
d101 45
a145 1
		int baud;
d147 14
a160 2
		baud = atoi(val ? val : "9600");
		rs232_baud(baud);
d163 1
a163 1
		 * Not a field we support...
@
