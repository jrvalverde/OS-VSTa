head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.2
date	93.04.23.22.40.42;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.10.18.09.42;	author vandys;	state Exp;
branches;
next	;


desc
@Byte I/O commands
@


1.2
log
@Implement KDB
@
text
@/*
 * dbgio.c
 *	I/O port access
 */
#ifdef KDB
extern char *strchr();

/*
 * dbg_inport()
 *	Input a byte from a port
 */
void
dbg_inport(char *p)
{
	int x, y;

	x = get_num(p);
	y = inportb(x);
	printf("0x%x -> 0x%x\n", x, y);
}

/*
 * dbg_outport()
 *	Output a byte to a port
 */
void
dbg_outport(char *p)
{
	char *val;
	int x, y;

	val = strchr(p, ' ');
	if (!val) {
		printf("Usage: outport <port> <value>\n");
		return;
	}
	*val++ = '\0';
	x = get_num(p);
	y = get_num(val);
	outportb(x, y);
	printf("0x%x <- 0x%x\n", x, y);
}

#endif /* KDB */
@


1.1
log
@Initial revision
@
text
@d5 1
a5 1
#if defined(DEBUG) || defined(KDB)
d44 1
a44 1
#endif /* DEBUG|KDB */
@
