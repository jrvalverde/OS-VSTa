head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.02.10.18.11.56;	author vandys;	state Exp;
branches;
next	;


desc
@Formatted ASCII dump, stolen from dbg/dump.c
@


1.1
log
@Initial revision
@
text
@/*
 * dump.c
 *	Dump memory in various formats
 */
#include <stdio.h>

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
 * dump_s()
 *	Dump strings
 */
void
dump_s(char *buf, uint count)
{
	int x, col = 0;

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
@
