head	1.9;
access;
symbols
	V1_3_1:1.5
	V1_3:1.4
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.9
date	94.12.21.05.30.21;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.10.13.17.54.52;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.09.23.20.39.37;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.05.30.21.31.57;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.04.26.21.38.24;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.03.15.22.06.52;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.19.21.40.29;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.31.04.34.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.16.19;	author vandys;	state Exp;
branches;
next	;


desc
@Port and portref definitions, state for servers and their clients
@


1.9
log
@Pass help proc sema to fork_ports so he doesn't have to hold
sema through all the FS_DUP's
@
text
@#ifndef _PORT_H
#define _PORT_H
/*
 * port.h
 *	Internal structure for a communication port
 */
#include <sys/types.h>
#include <sys/mutex.h>
#include <sys/seg.h>

struct port {
	lock_t p_lock;		/* Mutex for modifying port */
	struct sysmsg		/* FIFO list of messages */
		*p_hd,
		*p_tl;
	sema_t p_sema;		/* For serializing receivers */
	sema_t p_wait;		/* For sleeping to receive a message */
	ushort p_flags;		/* See below */
	struct portref		/* Linked list of references to this port */
		*p_refs;
	sema_t p_mapsema;	/* Mutex for p_maps */
	struct hash		/* Map file-ID -> pset */
		*p_maps;
	port_name p_name;	/* Name of this port */
};

/*
 * Bits in p_flags
 */
#define P_CLOSING 1		/* Port is shutting down */
#define P_ISR 2			/* Port has an ISR vectored to it */

/*
 * Flag value for struct port's p_map fields indicating that we're not
 * allowing a map hash to be created any more.
 */
#define NO_MAP_HASH ((struct hash *)1)

/*
 * Per-connected-port structure.  The protocol offered through
 * such ports requires that we maintain a mutex.
 */
struct portref {
	sema_t p_sema;		/* Only one I/O through a port at a time */
	struct port *p_port;	/* The port we access */
	lock_t p_lock;		/* Master mutex */
	sema_t p_iowait;	/* Where proc sleeps for I/O */
	sema_t p_svwait;	/*  ...server, while client copies out */
	ushort p_state;		/* See below */
	ushort p_flags;		/* Flags; see below also */
	struct sysmsg		/* The message descriptor */
		*p_msg;
	struct portref		/* Linked list of refs to a port */
		*p_next,
		*p_prev;
	struct segref		/* Segments mapped from server's msg_receive */
		p_segs;
};

/*
 * Special value for a portref pointer to reserve a slot
 */
#define PORT_RESERVED ((struct portref *)1)

/*
 * Values for p_state
 */
#define PS_IOWAIT 1	/* I/O sent, waiting for completion */
#define PS_IODONE 2	/*  ...received */
#define PS_ABWAIT 3	/* M_ABORT sent, waiting for acknowledgement */
#define PS_ABDONE 4	/*  ...received */
#define PS_OPENING 5	/* M_CONNECT sent */
#define PS_CLOSING 6	/* M_DISCONNECT sent */

/*
 * Bits in p_flags
 */
#define PF_NODUP (0x1)	/* Don't allow duplicates (dup/fork/etc) */

#ifdef KERNEL
extern struct portref *dup_port(struct portref *);
extern void fork_ports(sema_t *, struct portref **, struct portref **, uint);
extern struct portref *alloc_portref(void);
extern void shut_client(struct portref *);
extern int shut_server(struct port *);
extern void free_portref(struct portref *);
extern int kernmsg_send(struct portref *, int, long *);
extern struct port *alloc_port(void);
extern void exec_cleanup(struct port *);
extern struct portref *find_portref(struct proc *, port_t),
	*delete_portref(struct proc *, port_t);
extern struct port *find_port(struct proc *, port_t),
	*delete_port(struct proc *, port_t);
extern void mmap_cleanup(struct port *);
#endif

#endif /* _PORT_H */
@


1.8
log
@Add "don't dup" portref flag
@
text
@d82 1
a82 1
extern void fork_ports(struct portref **, struct portref **, uint);
@


1.7
log
@Clean up mmap() support
@
text
@d49 2
a50 1
	int p_state;		/* See below */
d74 5
@


1.6
log
@Move struct refs to central place (works better)
@
text
@d88 1
@


1.5
log
@Move NO_MAP_HASH out to .h so msgcon.c can use it for
an assertion
@
text
@a74 1
STRUCT_REF(proc);
@


1.4
log
@Add protos, as well as p_name field to record port_name
for a server port.
@
text
@d34 6
@


1.3
log
@Add FID->pset hash mapping.  Add flag so can see when
port is coming down.
@
text
@d24 1
d69 1
d79 4
@


1.2
log
@Get rid of unused field (actually, moved to portref but this
didn't get cleaned up).
@
text
@d21 3
d29 1
d71 2
a72 1
extern int shut_client(struct portref *), shut_server(struct port *);
d75 2
@


1.1
log
@Initial revision
@
text
@a18 2
	struct segref		/* Incoming messages mapped */
		p_segs;
@
