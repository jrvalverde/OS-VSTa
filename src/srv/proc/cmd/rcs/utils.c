head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.12.21.16.47.10;	author vandys;	state Exp;
branches;
next	;


desc
@Shared functions for /proc utilities
@


1.1
log
@Initial revision
@
text
@/*
 * utils.c
 *	Functions useful to both kill and ps
 */
#include "ps.h"
#include <mnttab.h>

extern port_t path_open(char *, int);

/*
 * mount_procfs()
 *	Mount /proc into our filesystem namespace
 */
void
mount_procfs(void)
{
	port_t port;
	
	port = path_open("fs/proc", ACC_READ);
	if (port < 0) {
		perror("/proc");
		exit(1);
	}
	(void)mountport("/proc", port);
}
@
