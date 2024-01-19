head	1.4;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2;
locks; strict;
comment	@ * @;


1.4
date	94.10.06.00.01.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.05.30.21.26.44;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.11.28.19.33.29;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.11.25.04.42.37;	author vandys;	state Exp;
branches;
next	;


desc
@Interrupt handling, some other UART handling as well
@


1.4
log
@Update server; lotsa new devices supported, lots new options
@
text
@/*
 * isr.c
 *	Handler for interrupt events
 */
#include "rs232.h"
#include "fifo.h"

extern uint iobase;
extern int txwaiters, rxwaiters;
extern struct fifo *inbuf, *outbuf;
extern uchar dtr, dsr, rts, cts, dcd, ri;

int txbusy;		/* UART sending data right now? */

/*
 * rs232_isr()
 *	Called to process an interrupt event from the port
 */
void
rs232_isr(struct msg *m)
{
	for (;;) {
		uchar why;
		uchar c;

		/*
		 * Decode next reason
		 */
		why = inportb(iobase + IIR) & IIR_IMASK;
		switch (why) {

		/*
		 * Line state, just clear
		 */
		case IIR_RLS:
			(void)inportb(iobase + LSR);
			break;

		/*
		 * Modem state, just clear
		 */
		case IIR_MLSC:
			c = inportb(iobase + MSR);
			dsr = (c & MSR_DSR) ? 1 : 0;
			cts = (c & MSR_CTS) ? 1 : 0;
			dcd = (c & MSR_DCD) ? 1 : 0;
			ri = (c & MSR_RI) ? 1 : 0;
			break;

		/*
		 * All done for this ISR
		 */
		case IIR_NOPEND:
			goto out;

		case IIR_RXTOUT:	/* Receiver ready */
		case IIR_RXRDY:
			c = inportb(iobase + DATA);
			fifo_put(inbuf, c);
			break;

		case IIR_TXRDY:		/* Transmitter ready */
			if (txwaiters && fifo_empty(outbuf)) {
				dequeue_tx();
			}
			if (!fifo_empty(outbuf)) {
				outportb(iobase + DATA,
					fifo_get(outbuf));
				txbusy = 1;
			} else {
				txbusy = 0;
			}
			break;
		}
	}
out:
	/*
	 * If we received any data, and somebody is waiting,
	 * call the hook to wake them up.
	 */
	if (!fifo_empty(inbuf) && rxwaiters) {
		dequeue_rx();
	}
}

/*
 * start_tx()
 *	Start transmitter
 */
void
start_tx(void)
{
	struct fifo *f = outbuf;

	if (fifo_empty(f) || txbusy) {
		return;
	}
	outportb(iobase + DATA, fifo_get(f));
	txbusy = 1;
}

/*
 * rs232_enable()
 *	Enable rs232 interrupts
 */
void
rs232_enable(void)
{
	/*
	 * Start with port set up for hard-wired RS-232
	 */
	outportb(iobase + MCR, MCR_DTR|MCR_RTS|MCR_IENABLE);
	dtr = 1;
	rts = 1;
	rs232_getinsigs();

	/*
	 * Allow all interrupt sources
	 */
	outportb(iobase + IER,
		IER_ERXRDY|IER_ETXRDY|IER_ERLS|IER_EMSC);
}
@


1.3
log
@Updates for support of RS-232 line control, syslog
@
text
@a109 10
	 * Set up 16550 FIFO chip, if present
	 */
	outportb(iobase + FIFO, FIFO_ENABLE|FIFO_RCV_RST|
		FIFO_XMT_RST|FIFO_TRIGGER_4);
	__msleep(100);
	if ((inportb(iobase + IIR) & IIR_FIFO_MASK) == IIR_FIFO_MASK) {
		outportb(iobase + FIFO, FIFO_ENABLE|FIFO_TRIGGER_4);
	}

	/*
@


1.2
log
@Turn on FIFO support, add some comments
@
text
@a4 2
#include <sys/msg.h>
#include <sys/types.h>
d11 1
d24 1
a24 1
		char c;
d43 5
a47 1
			(void)inportb(iobase + MSR);
a102 21
 * rs232_baud()
 *	Set baud rate
 *
 * Not really an ISR'ish thing, but it's the best place I could find.
 */
void
rs232_baud(int baud)
{
	uint bits;

	if (baud == 0) {
		return;
	}
	bits = COMBRD(baud);
	outportb(iobase + CFCR, CFCR_DLAB);
	outportb(iobase + BAUDHI, (bits >> 8) & 0xFF);
	outportb(iobase + BAUDLO, bits & 0xFF);
	outportb(iobase + CFCR, CFCR_8BITS);
}

/*
d123 3
@


1.1
log
@Initial revision
@
text
@d127 13
d141 4
@
