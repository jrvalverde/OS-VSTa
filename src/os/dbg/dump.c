#ifdef KDB
/*
 * dump.c
 *	Dump memory in various formats
 */
#include <sys/vas.h>
#include <sys/pview.h>
#include <sys/pset.h>
#include <sys/port.h>
#include <sys/fs.h>

extern char *strchr();
extern void *maploc();

static int dot,			/* Last position we examined */
	dcount = 0;		/*  ... last count */
static char lastfmt = 0;	/*  ... last format */

/*
 * prpad()
 *	Print hex number with leading 0 padding
 */
static void
prpad(unsigned long n, int len)
{
	char buf[16], *p;
	int x;

	p = buf+16;
	*--p = '\0';
	while (len-- > 0) {
		x = n & 0xF;
		n >>= 4;
		if (x < 10) {
			*--p = x + '0';
		} else {
			*--p = (x-10) + 'a';
		}
	}
	*--p = ' ';
	printf(p);
}

/*
 * get_num()
 *	Convert word to value
 *
 * Numbers, numbers with 0x, and numbers with 0 are treated accordingly.
 * Letters are looked up as a symbol in the symbol table.
 */
get_num(str)
	char *str;
{
	char *p;
	char buf[128];
	extern int yyval;
	extern char *expr_line, *expr_pos;

	/*
	 * Filter garbage
	 */
	if (!str || !str[0]) {
		return(0);
	}

	/*
	 * Trim at end of line or next blank
	 */
	strcpy(buf, str);
 	p = strchr(buf, ' ');
 	if (p) {
 		*p = '\0';
	}

	/*
	 * Set up for parse, and do the parse
	 */
 	expr_line = expr_pos = buf;
 	(void)yyparse();
 	return(yyval);
}

/*
 * dump_s()
 *	Dump strings
 */
static void
dump_s(off, count, phys)
	int off, count, phys;
{
	int x, col = 0;
	char *buf;

	buf = maploc(off, count*sizeof(char), phys);
	for (x = 0; x < count; ++x) {
		prpad(buf[x] & 0xFF, 2);
		if (++col >= 16) {
			int y;
			char c;

			printf(" ");
			for (y = 0; y < 16; ++y) {
				c = buf[x-15+y];
				if ((c < ' ') || (c >= 0x7F)) {
					c = '.';
				}
				putchar(c);
			}
			printf("\n");
			col = 0;
		}
	}
	if (col) {		/* Partial line */
		int y;

		for (y = col; y < 16; ++y) {
			printf("   ");
		}
		for (y = 0; y < col; ++y) {
			char c;

			c = buf[count-col+y];
			if ((c < ' ') || (c >= 0x7F)) {
				c = '.';
			}
			putchar(c);
		}
		printf("\n");
	}
}

/*
 * dump_X()
 *	Dump a bunch of hex longwords
 */
static void
dump_X(off, count, phys)
	int off, count, phys;
{
	int x, col = 0;
	long *buf;

	buf = maploc(off, count*sizeof(long), phys);
	for (x = 0; x < count; ++x) {
		prpad(buf[x], 8);
		if (++col >= 8) {
			printf("\n");
			col = 0;
		}
	}
	if (col) {
		printf("\n");
	}
}

/*
 * do_dump()
 *	Central workhorse to do the actual memory examination
 */
static void
do_dump(phys, args)
	int phys;
	char *args;
{
	char fmt;
	int count;

	/*
	 * Use last count, or 1 if no previous count
	 */
 	if (dcount > 0) {
 		count = dcount;
	} else {
		count = 1;
	}

	/*
	 * Use last format, or 'X' if no previous
	 */
 	if (lastfmt) {
 		fmt = lastfmt;
	} else {
		fmt = 'X';
	}

	/*
	 * If first arg non-numeric, use as format
	 */
	if (args && args[0] && !isdigit(args[0]) && (args[1] == ' ')) {
		fmt = args[0];
		args += 2;
	}

	/*
	 * Next arg is address, default to dot otherwise
	 */
	if (args && args[0]) {
		if ((args[0] == '-') && (args[1] == ' ')) {
			/* Placeholder to specify different count */
			args += 2;
		} else {
			dot = get_num(args);
			args = strchr(args, ' ');
			if (args) {
				args += 1;
			}
		}
	}

	/*
	 * Next arg is count, default is 1
	 */
 	if (args && args[0]) {
 		count = get_num(args);
 	}
	dcount = count;
	lastfmt = fmt;

	switch (fmt) {
	case 'X':
		dump_X(dot, count, phys);
		dot += count*4;
		break;
	case 's':
		dump_s(dot, count, phys);
		dot += count;
		break;
	default:
		printf("Unknown format: '%c'\n", fmt);
		break;
	}
}

/*
 * dump_phys()
 *	Dump memory given physical address
 */
dump_phys(args)
	char *args;
{
	do_dump(1, args);
}

/*
 * dump_virt()
 *	 Dump memory given kernel virtual address
 */
dump_virt(args)
	char *args;
{
	do_dump(0, args);
}

/*
 * dump_instr()
 *	Dump memory, virtual, using instruction format
 */
dump_instr(args)
	char *args;
{
	int count = 10, x;

	/*
	 * Next argument is address
	 */
	if (args && args[0]) {
		dot = get_num(args);
		args = strchr(args, ' ');
		if (args) {
			args += 1;
		}
	}

	/*
	 * Last argument is count
	 */
	if (args && args[0]) {
		count = get_num(args);
	}

	/*
	 * Dump out instructions
	 */
 	for (x = 0; x < count; ++x) {
 		extern char *symloc();

 		printf("%s\t", symloc(dot));
 		dot = db_disasm(dot, 0);
 	}
}

/*
 * do_dump_pview()
 *	Dump fields of a pview
 */
static void
do_dump_pview(struct pview *pv)
{
	printf(" pview @ 0x%x vaddr 0x%x len 0x%x off 0x%x\n",
		pv, pv->p_vaddr, pv->p_len, pv->p_off);
	printf("  vas 0x%x next 0x%x prot 0x%x pset 0x%x\n",
		pv->p_vas, pv->p_next, pv->p_prot, pv->p_set);
}

/*
 * dump_pview()
 *	Command interface to display a pview
 */
void
dump_pview(char *p)
{
	struct pview *pv;

	if (!p || !p[0]) {
		printf("Usage: pview <addr>\n");
		return;
	}
	pv = (struct pview *)get_num(p);
	do_dump_pview(pv);
}

/*
 * dump_vas()
 *	Dump out the vas at the given address
 */
void
dump_vas(char *p)
{
	struct vas *vas;
	struct pview *pv;

	if (!p || !p[0]) {
		printf("Bad address\n");
		return;
	}
	vas = (struct vas *)get_num(p);
	if (!vas) {
		printf("Bad address\n");
		return;
	}

	printf("vas @ 0x%x hat 0x%x\n", vas, &vas->v_hat);
	for (pv = vas->v_views; pv; pv = pv->p_next) {
		do_dump_pview(pv);
	}
}

/*
 * pset_type()
 *	Give pset type symbolically
 */
static char *
pset_type(int t)
{
	switch (t) {
	case PT_UNINIT: return("uninit");
	case PT_ZERO: return("zero");
	case PT_FILE: return("file");
	case PT_COW: return("cow");
	case PT_MEM: return("mem");
	default: return("???");
	}
}

/*
 * dump_pset()
 *	Dump out stuff on a pset
 */
void
dump_pset(char *p)
{
	struct pset *ps;
	struct perpage *pp;
	int x;

	if (!p || !p[0] || !(ps = (struct pset *)get_num(p))) {
		printf("Bad addr\n");
		return;
	}

	/*
	 * Print pset dope
	 */
	printf("len 0x%x off 0x%x type %s locks %d p_u 0x%x\n",
		ps->p_len, ps->p_off,
		pset_type(ps->p_type),
		ps->p_locks, &ps->p_u);
	printf(" swap 0x%x refs %d flags 0x%x perpage 0x%x\n",
		ps->p_swapblk, ps->p_refs, ps->p_flags, ps->p_perpage);
	printf(" ops 0x%x lock 0x%x lockwait 0x%x cowsets 0x%x\n",
		ps->p_ops, &ps->p_lock, &ps->p_lockwait,
		ps->p_cowsets);

	/*
	 * Print perpage dope
	 */
	if (!ps->p_perpage) {
		printf("No per-page???\n");
		return;
	}
	for (x = 0, pp = ps->p_perpage; x < ps->p_len; ++x,++pp) {
		int f = pp->pp_flags;

		if (x > 20) {
			printf("...\n");
			break;
		}
		printf("  pfn 0x%x lock 0x%x refs %d atl 0x%x",
			pp->pp_pfn, pp->pp_lock, pp->pp_refs, pp->pp_atl);
		if (f & PP_V) printf(" VALID");
		if (f & PP_COW) printf(" COW");
		if (f & PP_SWAPPED) printf(" SWAPPED");
		if (f & PP_BAD) printf(" BAD");
		if (f & PP_R) printf(" REF");
		if (f & PP_M) printf(" MOD");
		printf("\n");
	}
}

/*
 * dump_port()
 *	Dump out a message port
 */
void
dump_port(char *p)
{
	struct port *port;

	if (!p || !p[0] || !(port = (struct port *)get_num(p))) {
		printf("Bad addr\n");
		return;
	}
	printf("hd 0x%x tl 0x%x p_sema 0x%x p_wait 0x%x flags 0x%x\n",
		port->p_hd, port->p_tl, &port->p_sema, &port->p_wait,
		port->p_flags);
	printf("refs 0x%x\n", port->p_refs);
}

/*
 * prstate()
 *	Convert portref's state into string
 */
static char *
prstate(int x)
{
	switch (x) {
	case PS_IOWAIT:
		return("iowait");
	case PS_IODONE:
		return("iodone");
	case PS_ABWAIT:
		return("abwait");
	case PS_ABDONE:
		return("abdone");
	case PS_OPENING:
		return("opening");
	case PS_CLOSING:
		return("closing");
	default:
		return("???");
	}
}

/*
 * dump_ref()
 *	Dump out a portref
 */
void
dump_ref(char *p)
{
	struct portref *pr;

	if (!p || !p[0] || !(pr = (struct portref *)get_num(p))) {
		printf("Bad addr\n");
		return;
	}
	printf("port 0x%x lock 0x%x iowait 0x%x svwait 0x%x\n",
		pr->p_port, &pr->p_lock, &pr->p_iowait, &pr->p_svwait);
	printf(" state %s sysmsg 0x%x next/prev 0x%x/0x%x segs 0x%x\n",
		prstate(pr->p_state),
		pr->p_msg, pr->p_next, pr->p_prev, &pr->p_segs);
}

/*
 * opname()
 *	Map msg_op to symbolic name
 */
static void
opname(ulong op)
{
	if (op & M_READ) {
		printf("M_READ | ");
		op &= ~M_READ;
	}
	switch (op) {
#define OP(op) case op: printf(#op); break
	OP(M_CONNECT); OP(M_DISCONNECT); OP(M_DUP);
	OP(M_ABORT); OP(M_ISR); OP(M_TIME);
	OP(FS_OPEN); OP(FS_READ); OP(FS_SEEK); OP(FS_WRITE);
	OP(FS_REMOVE); OP(FS_STAT); OP(FS_WSTAT);
	OP(FS_ABSREAD); OP(FS_ABSWRITE); OP(FS_FID); OP(FS_RENAME);
	default: printf("0x%x", op); break;
	}
}

/*
 * dump_sysmsg()
 *	Dump system format of interprocess message
 */
void
dump_sysmsg(char *p)
{
	struct sysmsg *sm;

	if (!p || !p[0] || !(sm = (struct sysmsg *)get_num(p))) {
		printf("Bad addr\n");
		return;
	}
	printf("op: "); opname(sm->m_op);
	printf(" arg 0x%x arg1 0x%x nseg %d\n",
		sm->m_arg, sm->m_arg1, sm->m_nseg);
	if (sm->m_nseg) {
		int x;

		printf(" Segments:");
		for (x = 0; x < sm->m_nseg; ++x) {
			printf(" 0x%x", sm->m_seg[x]);
		}
		printf("\n");
	}
	printf(" sender 0x%x next 0x%x err '%s'\n",
		sm->m_sender, sm->m_next, sm->m_err);
}
#endif /* KDB */
