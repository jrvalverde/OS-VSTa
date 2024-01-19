head	1.10;
access;
symbols
	V1_3_1:1.9
	V1_3:1.8
	V1_2:1.8
	V1_1:1.8
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.10
date	94.11.11.18.06.18;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.19.00.28.22;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.10.25.23.21.14;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.07.02.22.04.02;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.23.22.40.42;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.13.17.11.43;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.31.04.36.18;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.05.00.37.48;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.10.18.09.35;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.43.48;	author vandys;	state Exp;
branches;
next	;


desc
@Main interface for debugger
@


1.10
log
@Add sysmsg dump
@
text
@#ifdef KDB
/*
 * crash.c
 *	Main routine for crash program
 */
#include <sys/types.h>
#include <mach/setjmp.h>

extern void dump_phys(), dump_virt(), dump_procs(), dump_pset(),
	dump_instr(), trace(), trapframe(), dump_vas(), dump_port(),
	dump_pview(), dump_thread(), dump_ref(), reboot(), memleaks(),
	dump_sysmsg();
extern void dbg_inport(), dbg_outport();
static void quit(), calc(), set(), set_mem();
extern int get_num();

jmp_buf dbg_errjmp;		/* For aborting on error */

/*
 * Table of commands
 */
struct {
	char *c_name;	/* Name of command */
	voidfun c_fn;	/* Function to process the command */
} cmdtab[] = {
	"=", calc,
	"btrace", trace,
	"calc", calc,
	"di", dump_instr,
	"dp", dump_phys,
	"dv", dump_virt,
	"inport", dbg_inport,
	"memleaks", memleaks,
	"outport", dbg_outport,
	"port", dump_port,
	"proc", dump_procs,
	"pset", dump_pset,
	"pview", dump_pview,
	"quit", quit,
	"ref", dump_ref,
	"reboot", reboot,
	"set", set,
	"sysmsg", dump_sysmsg,
	"tframe", trapframe,
	"thread", dump_thread,
	"trace", trace,
	"vas", dump_vas,
	"writemem", set_mem,
	0, 0
};

/*
 * strncmp()
 *	Compare with limited count
 *
 * Returns 1 on mismatch, 0 on equal
 */
static
strncmp(char *p1, char *p2, int n)
{
	int x;

	for (x = 0; x < n; ++x) {
		if (p1[x] != p2[x]) {
			return(1);
		}
		if (p1[x] == '\0') {
			return(0);
		}
	}
	return(0);
}

/*
 * strchr()
 *	Return first position w. this char
 *
 * Not needed by kernel proper, so we implement here
 */
char *
strchr(char *p, char c)
{
	while (c != *p) {
		if (*p++ == '\0') {
			return(0);
		}
	}
	return(p);
}

/*
 * quit()
 *	Bye bye
 */
static void
quit()
{
	longjmp(dbg_errjmp, 2);
}

/*
 * set_mem()
 *	Set memory word to value
 */
static void
set_mem(char *p)
{
	uint addr, val;

	addr = get_num(p);
	p = strchr(p, ' ');
	if (p == 0) {
		printf("Usage: writemem <addr> <val> [<val>...]\n");
		return;
	}
	*p++ = '\0';
	while (p) {
		while (*p == ' ') {
			++p;
		}
		val = get_num(p);
		*(uint *)addr = val;
		addr += sizeof(uint);
		p = strchr(p, ' ');
	}
}

/*
 * calc()
 *	Print out value in multiple formats
 */
static void
calc(str)
	char *str;
{
	int x;

	x = get_num(str);
	printf("%s 0x%x %d\n", symloc(x), x, x);
}

/*
 * do_cmd()
 *	Given command string, look up and invoke handler
 */
static void
do_cmd(str)
	char *str;
{
	int x, len, matches = 0, match;
	char *p;

	p = strchr(str, ' ');
	if (p)
		len = p-str;
	else
		len = strlen(str);

	for (x = 0; cmdtab[x].c_name; ++x) {
		if (!strncmp(cmdtab[x].c_name, str, len)) {
			++matches;
			match = x;
		}
	}
	if (matches == 0) {
		printf("No such command\n");
		return;
	}
	if (matches > 1) {
		printf("Ambiguous\n");
		return;
	}
	(*cmdtab[match].c_fn)(p ? p+1 : p);
}

dbg_main(void)
{
	char cmd[128], lastcmd[16];

 	/*
 	 * Top-level error catcher
 	 */
	switch (setjmp(dbg_errjmp)) {
	case 0:
		break;
	case 1:
 		printf("Reset to command mode\n");
		break;
	case 2:
		printf("[Returning from debugger]\n");
		return;
	}

 	/*
 	 * Command loop
 	 */
 	for (;;) {
		extern void gets();
		gets(cmd);

		/*
		 * Keep last command, insert it if they just hit return
		 */
		if (!cmd[0] && lastcmd[0]) {
			strcpy(cmd, lastcmd);
		} else {
			char *p, *q, c;

			p = cmd;	/* Copy up to first space */
			q = lastcmd;
			while (c = *p++) {
				if (c == ' ')
					break;
				*q++ = c;
			}
			*q = '\0';
		}
		do_cmd(cmd);
	}
	return(0);
}

/*
 * yyerror()
 *	Report syntax error and reset
 */
yyerror()
{
	printf("Syntax error in expression\n");
	longjmp(dbg_errjmp, 1);
}

/*
 * set()
 *	Set a symbol to a value
 */
static void
set(s)
	char *s;
{
	off_t o;
	char *n = s;
	extern void setsym();

	s = strchr(s, ' ');
	if (!s) {
		printf("Usage: set <name> <value>\n");
		longjmp(dbg_errjmp, 1);
	}
	*s = '\0'; ++s;
	setsym(n, get_num(s));
}
#endif /* KDB */
@


1.9
log
@Memory leak command
@
text
@d11 2
a12 1
	dump_pview(), dump_thread(), dump_ref(), reboot(), memleaks();
d43 1
@


1.8
log
@Add reboot command for laptops without a reset button
@
text
@d11 1
a11 1
	dump_pview(), dump_thread(), dump_ref(), reboot();
d32 1
@


1.7
log
@gets() in the kernel doesn't return a value.  Also, gets() puts
its own prompt char, so don't emit one here.
@
text
@d11 1
a11 1
	dump_pview(), dump_thread(), dump_ref();
d39 1
@


1.6
log
@Implement KDB
@
text
@d194 2
a195 4
		printf(">");
		if (gets(cmd) == 0) {
			quit();
		}
@


1.5
log
@Add dump of portref
@
text
@d1 1
a1 1
#ifdef DEBUG
d251 1
a251 1
#endif
@


1.4
log
@Add dump_thread and a memory modification interface
@
text
@d11 1
a11 1
	dump_pview(), dump_thread();
d38 1
@


1.3
log
@Add pview dump interface
@
text
@d11 1
a11 1
	dump_pview();
d13 2
a14 1
static void quit(), calc(), set();
d40 1
d43 1
d97 27
a131 1
	extern int get_num();
d134 1
a134 2
	printf("%s 0x%x %d\n",
		symloc(x), x, x);
@


1.2
log
@Add I/O commands
@
text
@d10 2
a11 1
	dump_instr(), trace(), trapframe(), dump_vas(), dump_port();
d35 1
@


1.1
log
@Initial revision
@
text
@d11 1
d29 2
@
