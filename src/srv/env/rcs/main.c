head	1.10;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.7
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.10
date	94.06.21.20.58.35;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.05.30.21.29.10;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.02.28.22.04.51;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.07.09.18.34.16;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.13.17.13.01;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.12.23.30.00;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.25.17.04.44;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.45.58;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.05.16.02.28;	author vandys;	state Exp;
branches;
next	;


desc
@Main processing loop
@


1.10
log
@Convert to openlog()
@
text
@/*
 * main.c
 *	Main interface to environment server
 *
 * The name space is a tree, with each node possessing a name and
 * protection label.  If the node is internal, it can contain zero
 * or more entries.  If it is a leaf, it will reference a string.
 */
#include <sys/perm.h>
#include <sys/param.h>
#include "env.h"
#include <hash.h>
#include <sys/fs.h>
#include <sys/ports.h>
#include <sys/types.h>
#include <stdio.h>
#include <std.h>
#include <syslog.h>

port_t envport;	/* Port we receive contacts through */

static struct hash *filehash;
struct node rootnode;

/*
 * Default protection for system-defined names; anybody can read,
 * only sys can write.
 */
static struct prot env_prot = {
	1,
	ACC_READ,
	{1},
	{ACC_WRITE|ACC_CHMOD}
};

/*
 * env_seek()
 *	Set file position
 */
static void
env_seek(struct msg *m, struct file *f)
{
	if (m->m_arg < 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	f->f_pos = m->m_arg;
	m->m_buflen = m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * access()
 *	See if user is compatible with desired access for object
 */
access(struct file *f, int mode, struct prot *prot)
{
	int uperms, desired;

	uperms = perm_calc(f->f_perms, f->f_nperm, prot);
	desired = mode & (ACC_WRITE|ACC_READ|ACC_CHMOD);
	if ((uperms & desired) != desired) {
		return(1);
	}
	f->f_mode = uperms;
	return(0);
}

/*
 * new_client()
 *	Create new per-connect structure
 */
static void
new_client(struct msg *m)
{
	struct file *f;

	/*
	 * Get data structure
	 */
	if ((f = malloc(sizeof(struct file))) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Fill in fields.  Note that our buffer is the
	 * information on the permissions our client
	 * possesses.  For an M_CONNECT, the message is
	 * from the kernel, and trusted.
	 *
	 * As soon as possible, check access modes and bomb
	 * if they're bad.
	 */
	f->f_nperm = m->m_buflen/sizeof(struct perm);
	bcopy(m->m_buf, &f->f_perms, f->f_nperm*sizeof(struct perm));
	if (access(f, m->m_arg, &env_prot)) {
		msg_err(m->m_sender, EPERM);
		free(f);
		return;
	}
	/* f->f_mode set by access() */
	f->f_pos = 0L;

	/*
	 * Hash under the sender's handle
	 */
        if (hash_insert(filehash, m->m_sender, f)) {
		free(f);
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Start out at root node.  No home until set.  We are the
	 * first member of this group.
	 */
	f->f_node = &rootnode;
	f->f_home = 0;
	ref_node(&rootnode);
	f->f_forw = f->f_back = f;

	/*
	 * Return acceptance
	 */
	msg_accept(m->m_sender);
}

/*
 * dup_client()
 *	Duplicate current file access onto new session
 *
 * This is more of a Plan9 clone operation.  The intent is
 * to not share a struct file, so that when you walk it down
 * a level or seek it, you don't affect the thing you cloned
 * off from.
 *
 * This is a kernel-generated message; the m_sender is the
 * current user; m_arg specifies a handle which will be used
 * if we complete the operation with success.
 */
static void
dup_client(struct msg *m, struct file *fold)
{
	struct file *f;

	/*
	 * Get data structure
	 */
	if ((f = malloc(sizeof(struct file))) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Bulk copy
	 */
	*f = *fold;
	ref_node(f->f_node);
	ref_node(f->f_home);

	/*
	 * Hash under the sender's handle
	 */
	if (hash_insert(filehash, m->m_arg, f)) {
		deref_node(f->f_home);
		deref_node(f->f_node);
		free(f);
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Join group
	 */
	f->f_forw = fold;
	f->f_back = fold->f_back;
	fold->f_back->f_forw = f;
	fold->f_back = f;

	/*
	 * Return acceptance
	 */
	m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * dead_client()
 *	Someone has gone away.  Free their info.
 */
static void
dead_client(struct msg *m, struct file *f)
{
	/*
	 * Remove from client hash, release ref to our
	 * current node.
	 */
	(void)hash_delete(filehash, m->m_sender);
	deref_node(f->f_node);

	/*
	 * If we're the last in the group, tear down our
	 * home node.
	 */
	if (f->f_forw == f) {
		remove_node(f->f_home);
	} else {
		/*
		 * Otherwise leave the group and drop our
		 * reference.
		 */
		f->f_back->f_forw = f->f_forw;
		f->f_forw->f_back = f->f_back;
		deref_node(f->f_home);
	}
	free(f);
}

/*
 * env_main()
 *	Endless loop to receive and serve requests
 */
static void
env_main()
{
	struct msg msg;
	int x;
	struct file *f;

loop:
	/*
	 * Receive a message, log an error and then keep going
	 */
	x = msg_receive(envport, &msg);
	if (x < 0) {
		syslog(LOG_ERR, "msg_receive");
		goto loop;
	}

	/*
	 * All requests should fit in one buffer
	 */
	if (msg.m_nseg > 1) {
		msg_err(msg.m_sender, EINVAL);
		goto loop;
	}

	/*
	 * Categorize by basic message operation
	 */
	f = hash_lookup(filehash, msg.m_sender);
	switch (msg.m_op) {
	case M_CONNECT:		/* New client */
		new_client(&msg);
		break;
	case M_DISCONNECT:	/* Client done */
		dead_client(&msg, f);
		break;
	case M_DUP:		/* File handle dup during exec() */
		dup_client(&msg, f);
		break;
	case M_ABORT:		/* Aborted operation */
		/*
		 * We're synchronous, so presumably the operation
		 * is all done and this abort is old news.
		 */
		msg_reply(msg.m_sender, &msg);
		break;
	case FS_OPEN:		/* Look up file from directory */
		if (!valid_fname(msg.m_buf, x)) {
			msg_err(msg.m_sender, ESRCH);
			break;
		}
		env_open(&msg, f);
		break;
	case FS_READ:		/* Read file */
		env_read(&msg, f, x);
		break;
	case FS_WRITE:		/* Write file */
		env_write(&msg, f, x);
		break;
	case FS_SEEK:		/* Set new file position */
		env_seek(&msg, f);
		break;
	case FS_REMOVE:		/* Get rid of a file */
		env_remove(&msg, f);
		break;
	case FS_STAT:		/* Stat node */
		env_stat(&msg, f);
		break;
	case FS_WSTAT:		/* Write stat info */
		env_wstat(&msg, f);
		break;
	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
	}
	goto loop;
}

/*
 * main()
 *	Startup of environment manager
 */
main()
{
	/*
	 * Initialize syslog
	 */
	openlog("env", LOG_PID, LOG_DAEMON);

	/*
	 * Allocate data structures we'll need
	 */
        filehash = hash_alloc(16);
	if (filehash == 0) {
		syslog(LOG_ERR, "file hash not allocated");
		exit(1);
        }

	/*
	 * Set up root node
	 */
	rootnode.n_prot = env_prot;
	rootnode.n_up = 0;
	strcpy(rootnode.n_name, "/");
	rootnode.n_flags = N_INTERNAL;
	ll_init(&rootnode.n_elems);
	rootnode.n_refs = 1;	/* Always at least 1 */
	rootnode.n_owner = 0;

	/*
	 * Register our well-known address
	 */
	envport = msg_port(PORT_ENV, 0);
	if (envport < 0) {
		syslog(LOG_ERR, "can't register name");
		exit(1);
	}

	syslog(LOG_INFO, "environment manager started");

	/*
	 * Start serving requests for the filesystem
	 */
	env_main();
}
@


1.9
log
@Syslog support
@
text
@a23 1
char env_sysmsg[] = "env (ENV):";
d237 1
a237 1
		syslog(LOG_ERR, "%s msg_receive", env_sysmsg);
d309 5
d318 1
a318 1
		syslog(LOG_ERR, "%s file hash not allocated", env_sysmsg);
d338 1
a338 1
		syslog(LOG_ERR, "%s can't register name", env_sysmsg);
d342 1
a342 1
	syslog(LOG_INFO, "%s environment manager started", env_sysmsg);
@


1.8
log
@Remove obsolete debugging
@
text
@d18 1
d24 1
d238 1
a238 1
		perror("env: msg_receive");
d314 1
a314 1
		perror("file hash");
d334 1
a334 1
		fprintf(stderr, "env: can't register name\n");
d337 2
@


1.7
log
@Source reorg
@
text
@a306 2
	port_t port;

a334 11

#ifdef DEBUG
	/*
	 * XXX hack so printf will show up on console
	 */
	port = msg_connect(PORT_KBD, ACC_READ);
	__fd_alloc(port);
	port = msg_connect(PORT_CONS, ACC_WRITE);
	__fd_alloc(port);
	__fd_alloc(port);
#endif
@


1.6
log
@Boot args work
@
text
@d11 2
a12 2
#include <env/env.h>
#include <lib/hash.h>
@


1.5
log
@Set name
@
text
@d305 1
a305 1
main(int argc, char *argv[])
a307 7

	/*
	 * Set our name if we're standalone
	 */
	if (argc == 0) {
		set_cmd("env");
	}
@


1.4
log
@Add UID tag, get from client
@
text
@d310 7
@


1.3
log
@General tidy-up
@
text
@d327 1
@


1.2
log
@New arg for msg_port
@
text
@d17 1
a18 4
extern void *malloc(), env_open(), env_read(), env_write(),
	env_remove(), env_stat(), env_wstat();
extern char *strerror();

d22 1
a22 1
static struct node rootnode;
a41 2
	struct node *n = f->f_node;

d61 1
a61 1
	if ((uperms & desired) != desired)
d63 1
d114 2
a115 1
	 * Start out at root node
d118 3
a120 1
	rootnode.n_refs += 1;
d158 2
d164 3
a166 1
        if (hash_insert(filehash, m->m_arg, f)) {
d173 1
a173 1
	 * Bump ref now that the has_insert worked
d175 4
a178 1
	f->f_node->n_refs += 1;
d194 4
d199 17
a215 1
	f->f_node->n_refs -= 1;
d243 1
a243 1
	if ((msg.m_nseg > 1) && !FS_RW(msg.m_op)) {
a307 3
	port_name blkname;
	struct msg msg;
	int chan, fd, x;
d324 1
a324 1
	rootnode.n_internal = 1;
d329 1
a329 2
	 * Block device looks good.  Last check is that we can register
	 * with the given name.
d332 2
a333 2
	if (x < 0) {
		fprintf(stderr, "ENV: can't register name\n");
@


1.1
log
@Initial revision
@
text
@d309 1
a309 1
	envport = msg_port(PORT_ENV);
@
