head	1.13;
access;
symbols
	V1_3_1:1.12
	V1_3:1.12
	V1_2:1.12
	V1_1:1.12
	V1_0:1.11;
locks; strict;
comment	@ * @;


1.13
date	94.12.19.05.51.49;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.08.29.22.55.54;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.06.30.19.55.12;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.05.03.21.31.52;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.04.23.22.42.19;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.04.12.23.31.57;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.04.12.23.29.22;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.12.20.57.29;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.27.00.33.04;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.24.19.12.54;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.20.00.24.05;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.18.17.42;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.51.11;	author vandys;	state Exp;
branches;
next	;


desc
@Miscellany--panic, printf, string functions, etc.
@


1.13
log
@Recalculate protections when a new identity is chosen
@
text
@/*
 * misc.c
 *	Miscellaneous support routines
 */
#include <sys/types.h>
#include <sys/assert.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/fs.h>

#define NUMBUF (16)	/* Buffer size for numbers */

extern void putchar();
char *strcpy(char *, const char *);

/*
 * get_ustr()
 *	Get a counted user string, enforce sanity
 */
get_ustr(char *kstr, int klen, void *ustr, int ulen)
{
	int x;

	if ((ulen+1) > klen) {
		return(err(EINVAL));
	}
	if (copyin(ustr, kstr, ulen)) {
		return(err(EFAULT));
	}
	kstr[ulen] = '\0';
	if (strlen(kstr) < 1) {
		return(err(EINVAL));
	}
	return(0);
}

/*
 * num()
 *	Convert number to string
 */
static void
num(char *buf, uint x, uint base)
{
	char *p = buf+NUMBUF;
	uint c, len = 1;

	*--p = '\0';
	do {
		c = (x % base);
		if (c < 10) {
			*--p = '0'+c;
		} else {
			*--p = 'a'+(c-10);
		}
		len += 1;
		x /= base;
	} while (x != 0);
	bcopy(p, buf, len);
}

/*
 * puts()
 *	Print out a string
 */
void
puts(char *s)
{
	char c;

	while (c = *s++) {
		putchar(c);
	}
}

/*
 * printf()
 *	Very small subset printf()
 */
static void
do_print(char *buf, char *fmt, int *args)
{
	char *p = fmt, c;
	char numbuf[NUMBUF];

	while (c = *p++) {
		if (c != '%') {
			*buf++ = c;
			continue;
		}
		switch (c = *p++) {
		case 'd':
			num(numbuf, *args++, 10);
			strcpy(buf, numbuf);
			buf += strlen(buf);
			break;
		case 'x':
			num(numbuf, *args++, 16);
			strcpy(buf, numbuf);
			buf += strlen(buf);
			break;
		case 's':
			strcpy(buf, (char *)(*args++));
			buf += strlen(buf);
			break;
		default:
			*buf++ = c;
			break;
		}
	}
	*buf = '\0';
}

/*
 * sprintf()
 *	Print into a string
 */
void
sprintf(char *buf, char *fmt, int arg1, ...)
{
	do_print(buf, fmt, &arg1);
}

/*
 * printf()
 *	Print onto console
 */
void
printf(char *fmt, int arg1, ...)
{
	char buf[132];

	do_print(buf, fmt, &arg1);
	puts(buf);
}

/*
 * panic()
 *	Print message and crash
 */
void
panic(char *msg, int arg1, ...)
{
	char buf[132];

	cli();
	do_print(buf, msg, &arg1);
	puts(buf);
	printf("\n", 0);
	for (;;) {
#ifdef KDB
		extern void dbg_enter();

		dbg_enter();
#else
		extern void nop();

		nop();
#endif
	}
}

/*
 * err()
 *	Central routine to record error for current thread
 *
 * Always returns -1, which is the return value for a syscall
 * which has an error.
 */
err(char *msg)
{
	extern void mach_flagerr();

	strcpy(curthread->t_err, msg);
	mach_flagerr(curthread->t_uregs);
	return(-1);
}

/*
 * strcpy()
 *	We don't use the C library, so need to provide our own
 */
char *
strcpy(char *dest, const char *src)
{
	while (*dest++ = *src++)
		;
	return(0);
}

/*
 * strlen()
 *	Length of string
 */
int
strlen(const char *p)
{
	int x = 0;

	while (*p++)
		++x;
	return(x);
}

/*
 * canget()
 *	Common function to estimate how powerful a thread is
 */
static int
canget(int bit)
{
	int perms;
	static struct prot rootprot =
		{2, 0, {1, 1}, {ACC_READ, ACC_WRITE}};

	perms = perm_calc(curthread->t_proc->p_ids, PROCPERMS, &rootprot);
	if (!(perms & bit)) {
		err(EPERM);
		return(0);
	}
	return(1);
}

/*
 * isroot()
 *	Tell if the current thread's a big shot
 *
 * Sets err(EPERM) if he isn't.
 */
isroot(void)
{
	return(canget(ACC_WRITE));
}

/*
 * issys()
 *	Like root, but little shots OK too...
 */
issys(void)
{
	return(canget(ACC_READ));
}

/*
 * memcpy()
 *	The compiler can generate these; we use bcopy()
 */
void *
memcpy(void *dest, const void *src, size_t cnt)
{
	bcopy(src, dest, cnt);
	return(dest);
}

/*
 * strcmp()
 *	Compare two strings, return whether they're equal
 *
 * We don't bother with the distinction of which is "less than"
 * the other.
 */
int
strcmp(const char *s1, const char *s2)
{
	while (*s1++ == *s2) {
		if (*s2++ == '\0') {
			return(0);
		}
	}
	return(1);
}

/*
 * strerror()
 *	Return current error string to user
 */
strerror(char *ustr)
{
	int len;

	len = strlen(curthread->t_err);
	if (copyout(ustr, curthread->t_err, len+1)) {
		return(err(EFAULT));
	}
	return(0);
}

/*
 * perm_ctl()
 *	Set a slot to given permission value, or read slot
 */
perm_ctl(int arg_idx, struct perm *arg_perm, struct perm *arg_ret)
{
	uint x;
	struct perm perm;
	struct proc *p = curthread->t_proc;

	/*
	 * Legal slot?
	 */
	if ((arg_idx < 0) || (arg_idx >= PROCPERMS)) {
		return(err(EINVAL));
	}

	/*
	 * Trying to set?
	 */
	if (arg_perm) {
		/*
		 * Argument OK?
		 */
		if (copyin(arg_perm, &perm, sizeof(struct perm))) {
			return(err(EFAULT));
		}

		/*
		 * See if any of our current permissions dominates it
		 */
		p_sema(&p->p_sema, PRILO);
		for (x = 0; x < PROCPERMS; ++x) {
			if (perm_dominates(&p->p_ids[x], &perm)) {
				break;
			}
		}

		/*
		 * If we found one, then we can overwrite the label.
		 * Preserve the UID which allowed this.
		 */
		if (x < PROCPERMS) {
			p->p_ids[arg_idx] = perm;
			if (p->p_ids[x].perm_uid) {
				p->p_ids[arg_idx].perm_uid =
					p->p_ids[x].perm_uid;
			}
		}
		
		/*
		 * If we just changed our default ownership make our
		 * protections match our new self
		 */
		if (arg_idx == 0) {
			int i, plen;

			plen = PERM_LEN(&perm);
			p->p_prot.prot_bits[plen]
				= p->p_prot.prot_bits[p->p_prot.prot_len];
			p->p_prot.prot_len = plen;
			p->p_prot.prot_id[plen - 1]
				= p->p_ids[0].perm_id[plen - 1];
			for (i = 0; i < plen - 1; i++) {
				p->p_prot.prot_bits[i] = 0;
				p->p_prot.prot_id[i] = p->p_ids[0].perm_id[i];
			}
		}
		
		v_sema(&p->p_sema);

		/*
		 * Return result
		 */
		if (x >= PROCPERMS) {
			return(err(EPERM));
		}
	}

	/*
	 * Want a copy of the slot?
	 * XXX worth locking again to avoid race?
	 */
	if (arg_ret) {
		if (copyout(arg_ret, &p->p_ids[arg_idx],
				sizeof(struct perm))) {
			return(err(EFAULT));
		}
	}
	return(0);
}

/*
 * set_cmd()
 *	Set p_cmd[] field of proc
 *
 * Merely an advisory tool; it isn't trusted in any way
 */
set_cmd(char *arg_cmd)
{
	struct proc *p = curthread->t_proc;

	if (copyin(arg_cmd, p->p_cmd, sizeof(p->p_cmd))) {
		return(err(EFAULT));
	}
	return(0);
}

/*
 * getid()
 *	Get PID, TID, or PPID
 */
pid_t
getid(int which)
{
	pid_t pid;

	/*
	 * Easy cases
	 */
	switch (which) {
	case 0:
		return(curthread->t_proc->p_pid);
	case 1:
		return(curthread->t_pid);
	case 2:
		pid = parent_exitgrp(curthread->t_proc->p_parent);
		return((pid > 0) ? pid : 1);
		break;
	default:
		return(err(EINVAL));
	}
}

/*
 * assfail()
 *	Assertion failure in kernel
 */
void
assfail(const char *msg, const char *file, int line)
{
	printf("Assertion failed line %d file %s\n",
		line, file);
	panic((char *)msg, 0);
}

/*
 * __main()
 *	Satisfies a GCC reference hook for C++ support
 */
void
__main(void)
{
}
@


1.12
log
@Map assfail() onto ANSI prototype, fix const handling
@
text
@d337 20
@


1.11
log
@GCC warning cleanup
@
text
@d407 1
a407 1
assfail(char *msg, char *file, uint line)
d411 1
a411 1
	panic(msg, 0);
@


1.10
log
@Put kernel-specific assertion failure routine in kernel itself
@
text
@d15 1
a15 1
void strcpy();
d183 2
a184 2
void
strcpy(char *dest, char *src)
d188 1
d195 2
a196 1
strlen(char *p)
d249 1
a249 1
memcpy(void *dest, void *src, uint cnt)
d262 2
a263 1
strcmp(char *s1, char *s2)
d412 9
@


1.9
log
@Implement KDB
@
text
@d398 12
@


1.8
log
@Get rid of old debug printf
@
text
@d14 1
a14 1
extern void dbg_enter(), putchar();
d151 3
d155 5
@


1.7
log
@When forging new ability, keep UID of enabling
ability if it's present.
@
text
@a319 1
			printf("perm_ctl uid %d\n", perm.perm_uid);
a321 2
				printf(" ...inherit %d\n",
					p->p_ids[x].perm_uid);
@


1.6
log
@Random ID interface
@
text
@d316 2
a317 1
		 * If we found one, then we can overwrite the label
d320 1
d322 6
@


1.5
log
@Add p_cmd to hold a short program name per process
@
text
@d359 26
@


1.4
log
@Flag error through machine-dependent code.  Fix perm_ctl() to
match prototype, and add extra check to signed value.
@
text
@d343 16
@


1.3
log
@Add a utility for fiddling the permission tables
@
text
@d164 2
d167 1
d281 1
a281 1
perm_ctl(uint arg_idx, struct perm *arg_perm, struct perm *arg_ret)
d290 1
a290 1
	if (arg_idx >= PROCPERMS) {
@


1.2
log
@Get rid of stray '&'
@
text
@d273 67
@


1.1
log
@Initial revision
@
text
@d203 1
a203 1
	perms = perm_calc(&curthread->t_proc->p_ids, PROCPERMS, &rootprot);
@
