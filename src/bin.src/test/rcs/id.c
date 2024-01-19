head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.4
date	93.05.25.02.26.33;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.23.54.20;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.30.40;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.24.00.38.51;	author vandys;	state Exp;
branches;
next	;


desc
@Little utility to find out what we can do
@


1.4
log
@Print root ID specially
@
text
@/*
 * id.c
 *	Print out our current IDs
 */
#include <sys/param.h>
#include <sys/perm.h>

main()
{
	int x, y, disabled, printed = 0;
	struct perm perm;

	/*
	 * Get ID and print
	 */
	for (x = 0; x < PROCPERMS; ++x) {
		/*
		 * Get next slot
		 */
		if (perm_ctl(x, (void *)0, &perm) < 0) {
			continue;
		}

		/*
		 * Clear disabled flag, then see if this slot
		 * has anything worth printing.
		 */
		disabled = 0;
		if (!PERM_ACTIVE(&perm)) {
			if (PERM_DISABLED(&perm)) {
				disabled = 1;
				PERM_ENABLE(&perm);
			} else {
				continue;
			}
		}

		/*
		 * Comma separated
		 */
		if (printed > 0) {
			printf(", ");
		}
		printed++;

		/*
		 * Print digits or <root>
		 */
		if (PERM_LEN(&perm) == 0) {
			printf("<root>");
		} else {
			for (y = 0; y < PERM_LEN(&perm); ++y) {
				if (y > 0) {
					printf(".");
				}
				printf("%d", perm.perm_id[y]);
			}
		}

		/*
		 * Print UID tag if present
		 */
		if (perm.perm_uid) {
			printf("(%d)", perm.perm_uid);
		}

		/*
		 * Valid but currently disabled
		 */
		if (disabled) {
			printf("(disabled)");
		}
	}
	printf("\n");
	return(0);
}
@


1.3
log
@Printed UID in wrong place
@
text
@d47 1
a47 1
		 * Print digits
d49 8
a56 3
		for (y = 0; y < PERM_LEN(&perm); ++y) {
			if (y > 0) {
				printf(".");
a57 1
			printf("%d", perm.perm_id[y]);
@


1.2
log
@Print UID tag if present
@
text
@d54 7
a60 3
			if (perm.perm_uid) {
				printf("(%d)", perm.perm_uid);
			}
@


1.1
log
@Initial revision
@
text
@d54 3
@
