head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.3
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	94.10.23.17.44.28;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.06.21.59.04;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.01.15.02.11.59;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.24.22.27.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.58.11;	author vandys;	state Exp;
branches;
next	;


desc
@Common code for doing stat-style messages
@


1.5
log
@Use prototypes
@
text
@/*
 * statsup.c
 *	Common functionality for writing stat fields
 */
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include <string.h>

extern int perm_set();

/*
 * do_wstat()
 *	Allow writing of supported stat messages
 *
 * Our return value is 0 if we handled the message, or 1 if the caller
 * needs to handle it.  For 1, the "field" and "val" pointers have been
 * set to the appropriate values.
 *
 * I'm not thrilled with the number of parameters, but I really want to
 * share all this tedious code for the common stat usages.
 */
do_wstat(struct msg *m, struct prot *prot,
	int acc, char **fieldp, char **valp)
{
	char *p, *end, *field, *val = 0;
	int len;
	static char buf[MAXSTAT];

	/*
	 * Sanity checks
	 */
	if (!(acc & ACC_CHMOD)) {
		msg_err(m->m_sender, EPERM);
		return(0);
	}
	if ((m->m_nseg != 1) || (m->m_buflen > MAXSTAT)) {
		msg_err(m->m_sender, EINVAL);
		return(0);
	}
	bcopy(m->m_buf, buf, m->m_buflen);
	buf[MAXSTAT-1] = '\0';

	/*
	 * Get field name part
	 */
	field = p = buf;
	end = p + m->m_buflen;
	while ((*p != '\n') && (*p != '=') && (p < end)) {
		++p;
	}
	if (p >= end) {
		msg_err(m->m_sender, EINVAL);
		return(0);
	}

	/*
	 * If there's a field value part, get it too
	 */
	if (*p == '=') {
		*p++ = '\0';
		val = p;
		while ((*p != '\n') && (p < end)) {
			++p;
		}
		if (p >= end) {
			msg_err(m->m_sender, EINVAL);
			return(0);
		}
		*p = '\0';
	} else if (*p == '\n') {
		*p = '\0';
	}

	/*
	 * Process each kind of field we can write
	 */

	if (!strcmp(field, "acc")) {
		/*
		 * Set default
		 */
		prot->prot_default = atoi(val);
		val = strchr(val, '/');
		if (val) {
			++val;
		}

		/*
		 * Set access bits
		 */
		(void)perm_set(prot->prot_bits, val);
	} else if (!strcmp(field, "perm")) {
		/*
		 * Set permission values.  Might change prot_len,
		 * so fix this field as well as cleaning up newly-used
		 * or -unused bits in prot_bits.
		 */
		len = perm_set(prot->prot_id, val);
		if (len != prot->prot_len) {
			int x;

			x = len;
			if (prot->prot_len < x)
				x = prot->prot_len;
			for ( ; x < PERMLEN; ++x)
				prot->prot_bits[x] = 0;
			prot->prot_len = len;
		}
	} else {
		/*
		 * Not a field we support...
		 */
		*fieldp = field;
		*valp = val;
		return(1);
	}

	/*
	 * Return success
	 */
	m->m_buflen = m->m_nseg = m->m_arg = 0;
	msg_reply(m->m_sender, m);
	return(0);
}
@


1.4
log
@File rename, to support libusr->libc merge
@
text
@d8 1
a9 1
extern char *strchr();
@


1.3
log
@Cleanup
@
text
@d2 1
a2 1
 * stat.c
@


1.2
log
@Fix up handling of buffer; can't write to one in message--it's
a read-only buffer mapped from the requestor.
@
text
@d13 1
a13 1
 * cons_wstat()
@


1.1
log
@Initial revision
@
text
@d28 1
d37 1
a37 1
	if (m->m_buflen > MAXSTAT) {
d41 2
d47 1
a47 1
	field = p = m->m_buf;
d85 1
a85 1
		if (val)
d87 1
@
