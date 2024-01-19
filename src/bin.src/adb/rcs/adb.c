head	1.11;
access;
symbols
	V1_3_1:1.10
	V1_3:1.10
	V1_2:1.9;
locks; strict;
comment	@ * @;


1.11
date	94.12.28.06.13.24;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.03.23.21.56.05;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.12.22.00.21.54;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.12.14.21.37.30;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.12.14.20.37.20;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.12.13.02.21.09;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.12.12.22.45.55;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.12.11.01.40.25;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.12.10.19.13.28;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.12.09.06.07.31;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.09.00.26.27;	author vandys;	state Exp;
branches;
next	;


desc
@Central code for getting commands, dumping, etc.
@


1.11
log
@Add symbol table expansion support
@
text
@/*
 * adb.c
 *	Main command handling for assembly debugger
 */
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <std.h>
#include "map.h"

#define MAXARGS (8)	/* Max # args to :r command */

extern char *getnum();
extern void breakpoints(void *, int), dump_breakpoints(void), dump_syms();

pid_t corepid;		/* PID we debug, if any */
port_t dbg_port;	/*  ...port we talk to him via */

uint addr, count = 1;	/* Addr/count for each command */
static FILE *objfile;	/* Open FILE * to object file */
static char *objname;	/*  ...name of this file */
jmp_buf errjmp;		/* Recovery from parse errors */
static struct map	/* Mappings for a.out and core */
	textmap,
	coremap;
static struct map	/* Points to a map to use */
	*read_from;

/* Macro to skip whitespace */
#define SKIP(p) {while (isspace(*(p))) p += 1; }

/*
 * usage()
 *	Show how to invoke adb
 */
static void
usage(void)
{
	printf("Usage is: adb <prog> [ <pid> ]\n");
	exit(1);
}

/*
 * readloc()
 */
ulong
readloc(ulong addr, int size)
{
	ulong off;
	uchar c;
	ushort w;
	ulong l;

	if (read_from == &coremap) {
		return(read_procmem(addr, size));
	} else {
		/*
		 * vaddr 0x1000 maps to offset 0 in an a.out
		 */
		if (do_map(read_from, (void *)addr, &off)) {
			return(0);
		}
		fseek(objfile, off, 0);

		/*
		 * Read required amount of data at this location
		 */
		switch (size) {
		case 1:	fread(&c, sizeof(c), 1, objfile); return(c);
		case 2:	fread(&w, sizeof(w), 1, objfile); return(w);
		case 4:	fread(&l, sizeof(l), 1, objfile); return(l);
		}
	}
}

/*
 * dollar_cmds()
 *	Handle $ commands
 */
static void
dollar_cmds(char *p)
{
	extern void dump_regs(void), backtrace(void);

	switch (*p) {
	case 'q':		/* Quit */
		exit(0);
	case 'r':		/* Dump registers */
		dump_regs();
		break;
	case 'b':		/* List breakpoints */
		dump_breakpoints();
		break;
	case 'c':		/* Show C stack backtrace */
		read_from = &coremap;
		backtrace();
		break;
	case 's':		/* Load additional symbol file */
		p += 1;
		while (*p && isspace(*p)) {
			p += 1;
		}
		if (*p) {
			rdsym(p);
		}
		break;
	case 'S':		/* Dump symbol table */
		dump_syms();
		break;
	default:
		printf("Unknown command: $%s\n", p);
	}
}

/*
 * do_dump()
 *	Actual workhorse to do dumping of memory
 *
 * Some hard-coded knowledge of word size here, sorry.
 */
static void
do_dump(char *fmt, int size, int cols)
{
	int x, col = 0;
	unsigned char c;
	unsigned short w;
	unsigned long l;

	for (x = 0; x < count; ++x) {
		switch (size) {
		case 1:
			c = readloc(addr, 1);
			printf(fmt, c);
			break;
		case 2:
			w = readloc(addr, 2);
			printf(fmt, w);
			break;
		case 4:
			l = readloc(addr, 4);
			printf(fmt, l);
			break;
		}
		col += 1;
		addr += size;
		if (col > cols) {
			printf("\n");
			col = 0;
		}
	}
	if (col) {
		printf("\n");
	}
}

/*
 * dump_s()
 *	Dump strings, with or without de-escaping
 */
static void
dump_s(int quote)
{
	uchar c;

	for (;;) {
		c = readloc(addr, sizeof(c));
		if ((c == '\0') || (c == '\n')) {
			printf("\n");
			break;
		}
		if (quote) {
			if (c & 0x80) {
				printf("M-");
				c &= ~0x80;
			}
			if (c < ' ') {
				putchar('^');
				c += 'A';
			}
			if (c == 0x7F) {
				printf("DEL");
			} else {
				putchar(c);
			}
		} else {
			putchar(c);
		}
		addr += 1;
	}
}

/*
 * dump_mem()
 *	Dump memory in a variety of formats
 */
static void
dump_mem(char *p)
{
	char fmt;
	int x, cnt, oldcnt;

	/*
	 * Optional count here as well.  The two interact in a way
	 * more complex than this in Sun's adb, but I can't figure
	 * it out.
	 */
	if (isdigit(*p)) {
		p = getnum(p, &cnt);
		SKIP(p);
		cnt *= count;
	} else {
		cnt = count;
	}
	oldcnt = count;
	count = cnt;

	/*
	 * Label location, start dumping
	 */
	printf("%s:\n", nameval(addr));
	fmt = *p++;
	switch (fmt) {
	case 'x': do_dump(" %4x", 2, 8); break;
	case 'X': do_dump(" %8X", 4, 4); break;
	case 'o': do_dump(" %8o", 2, 4); break;
	case 'O': do_dump(" %12O", 4, 4); break;
	case 'd': do_dump(" %5d", 2, 8); break;
	case 'D': do_dump(" %10d", 4, 4); break;
	case 'i':
		for (x = 0; x < count; ++x) {
			addr = db_disasm(addr, 0);
		}
		break;
	case 's': dump_s(0); break;
	case 'S': dump_s(1); break;
	case 'c': do_dump(" %02x", 1, 16); break;
	default:
		printf("Unknown format '%c'\n", fmt);
		return;
	}
	count = oldcnt;
}

/*
 * start()
 *	Run a process from the named object file
 */
static void
start(char *args)
{
	pid_t pid;
	port_name pn;
	port_t p;
	uint argc;
	char *argv[MAXARGS+1];

	/*
	 * Build arg list
	 */
	SKIP(args);
	argv[0] = objname;
	argc = 1;
	while (args && *args) {
		char *q;

		if (argc >= MAXARGS) {
			printf("Too many arguments\n");
			return;
		}
		argv[argc++] = args;
		q = strchr(args, ' ');
		if (q) {
			*q++ = '\0';
			SKIP(q);
		}
		args = q;
	}
	argv[argc] = 0;

	/*
	 * Launch a child.  We wait until our parent attaches.
	 */
	corepid = fork();
	if (corepid == 0) {
		uint cnt;

		/*
		 * Wait for parent to rendevous; bail if we haven't
		 * seen him in 60 seconds.
		 */
		cnt = 0;
		while (ptrace(0, 0) < 0) {
			__msleep(250);
			if (++cnt > 4*60) {
				_exit(1);
			}
		}

		/*
		 * Now run the named program
		 */
		execv(objname, argv);
		_exit(1);
	}

	/*
	 * Parent--attach to child, then let him sync to point of
	 * running new a.out.
	 */
	dbg_port = msg_port(0, &pn);
	if (dbg_port < 0) {
		perror("Debug port");
		exit(1);
	}
	if (ptrace(corepid, pn) < 0) {
		perror("ptrace attach");
		printf("Couldn't attach to %d on 0x%x\n", corepid, pn);
		exit(1);
	}
	wait_exec();
}

/*
 * colon_cmds()
 *	Do the ':' commands
 */
static void
colon_cmds(char *p)
{
	extern void run(void), step(void);

	switch (*p) {
	case 'c':
	case 's':
	case 'b':
	case 'd':
		if (!corepid) {
			printf("No process\n");
			return;
		}
	}
	read_from = &coremap;
	switch (*p) {
	case 'c':
		run();
		break;
	case 's':
		step();
		break;
	case 'b':
		breakpoint((void *)addr, 1);
		break;
	case 'd':
		breakpoint((void *)addr, 0);
		break;
	case 'r':
		if (corepid) {
			printf("Already running\n");
			break;
		}
		start(p+1);
		break;
	default:
		printf("Unknown command: :%s\n", p);
		break;
	}
}

/*
 * print_val()
 *	Print value of dot
 */
static void
print_val(char *p)
{
	char fmt[8];
	uint val;
	ulong vall;

	if (!*p) {
		p = "X";
	}
	switch (*p) {
	case 'd':
	case 'D':
	case 'x':
	case 'X':
	case 'o':
	case 'O':
		sprintf(fmt, "%%%c\n", *p);
		if (islower(*p)) {
			val = addr & 0xFFFF;
			printf(fmt, val);
		} else {
			vall = addr;
			printf(fmt, vall);
		}
		break;
	default:
		printf("Unknown format: %s\n", p);
		break;
	}
}

/*
 * cmdloop()
 *	Process commands
 *
 * General format is:
 *	<addr>,<count> <operator> <modifiers>
 *	<addr>, <count> are numbers or symbols
 *	<operator> is '/', '?', ':', '$'
 *	<modifiers> are specific to the operator
 */
static void
cmdloop(void)
{
	char buf[128], *p, c;
	static char lastcmd[32];

	/*
	 * Get a line, skip over white space
	 */
	write(1, ">", 1);
	gets(buf);
	p = buf;
	SKIP(p);

	/*
	 * Repeat last command on empty line
	 */
	if (!*p) {
		p = lastcmd;
	}

	/*
	 * If first field present, parse it.  Otherwise we inherit
	 * the address from the previous command.
	 */
	if (isalnum(*p) || (*p == '_')) {
		p = getnum(p, &addr);
		count = 1;	/* Count gets reset on explicit addr */
	}
	SKIP(p);

	/*
	 * Similarly for count
	 */
	if (*p == ',') {
		++p;
		SKIP(p);
		p = getnum(p, &count);
		SKIP(p);
	}

	/*
	 * Record last command so we can repeat
	 */
	if (p != lastcmd) {
		strncpy(lastcmd, p, sizeof(lastcmd)-1);
	}

	/*
	 * Pick out operator
	 */
	c = *p++;
	SKIP(p);
	switch (c) {
	case '?':
		read_from = &textmap;
		dump_mem(p);
		break;
	case '/':
		if (!corepid) {
			printf("No process attached\n");
			break;
		}
		read_from = &coremap;
		dump_mem(p);
		break;
	case ':':
		colon_cmds(p);
		break;
	case '$':
		dollar_cmds(p);
		break;
	case '=':
		print_val(p);
		break;
	default:
		printf("Unknown modifier: '%c'\n", c);
		break;
	}
}

/*
 * main()
 *	Start up
 *
 * We handle:
 *	adb <prog>
 *	adb <prog> [ <pid> ]
 */
main(int argc, char **argv)
{
	/*
	 * Check out arguments
	 */
	if ((argc < 2) || (argc > 3)) {
		usage();
	}
	if (argc == 3) {
		corepid = atoi(argv[2]);
	}

	/*
	 * Read symbols from a.out, leave him open so we can
	 * do "?" examination of him.
	 */
	if ((objfile = fopen(objname = argv[1], "r")) == NULL) {
		perror(argv[1]);
		exit(1);
	}
	rdsym(argv[1]);

	/*
	 * Now ask Mr. a.out to fill in our offsets
	 */
	map_aout(&textmap);

	/*
	 * If there's a process, set up for debugging him
	 */
	if (corepid) {
		port_name pn;
		port_t p;

		/*
		 * Create server port he will talk to us via
		 */
		dbg_port = msg_port(0, &pn);
		if (dbg_port < 0) {
			perror("Debug port");
			exit(1);
		}

		/*
		 * Tell ET to phone home.  He'll block until we start
		 * listening on our port.
		 */
		if (ptrace(corepid, pn) < 0) {
			perror("ptrace: attach");
			printf("Can't attach PID %d to 0x%x\n",
				corepid, pn);
			exit(1);
		}
	}

	/*
	 * For recovering from parse errors
	 */
	(void)setjmp(errjmp);

	/*
	 * Start processing commands
	 */
	for (;;) {
		cmdloop();
	}
}
@


1.10
log
@Check error return
@
text
@d15 1
a15 1
extern void breakpoints(void *, int), dump_breakpoints(void);
d98 12
@


1.9
log
@Fix treatment of multiple args to :r
@
text
@d304 5
a308 1
	ptrace(corepid, pn);
d540 6
a545 1
		ptrace(corepid, pn);
@


1.8
log
@Add better display of why proc broke into adb
@
text
@a152 1
	uint x;
d154 1
a154 1
	for (x = 0; x < count; ++x) {
d156 4
d177 1
a178 1
	addr += count;
d252 1
a252 1
	while (*args) {
d263 1
a263 2
			args = q;
			SKIP(args);
d265 1
@


1.7
log
@Add :r command
@
text
@d9 1
d12 2
d17 1
a17 1
static pid_t corepid;	/* PID we debug, if any */
d235 1
a235 1
start(void)
d240 25
d288 1
a288 1
		execl(objname, objname, (char *)0);
d343 1
a343 1
		start();
@


1.6
log
@Add C stack backtrace
@
text
@d19 1
d228 50
d288 11
a298 1
		read_from = &coremap;
a301 1
		read_from = &coremap;
d310 7
d378 1
a378 1
	printf("adb>"); fflush(stdout);
a435 4
		if (!corepid) {
			printf("No process attached\n");
			break;
		}
d474 1
a474 1
	if ((objfile = fopen(argv[1], "r")) == NULL) {
@


1.5
log
@Add breakpoint commands
@
text
@d80 1
a80 1
	extern void dump_regs(void);
d90 4
@


1.4
log
@Add single stepping and register dump
@
text
@d12 1
d88 3
d239 6
@


1.3
log
@Add register dump
@
text
@d225 15
a239 1
	/* TBD */
d297 1
@


1.2
log
@Add ptrace support
@
text
@d79 4
a82 1
	if (*p == 'q') {	/* Quit */
d84 5
a89 1
	printf("Unknown commands: $%s\n", p);
a403 1
		printf("port name 0x%x\n", pn);
@


1.1
log
@Initial revision
@
text
@d14 2
d46 3
d51 1
a51 1
		abort();
a52 4
		uchar c;
		ushort w;
		ulong l;

a68 1
		abort();
d396 3
a398 2
		p = msg_port(0, &pn);
		if (p < 0) {
@
