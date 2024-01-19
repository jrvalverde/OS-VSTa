head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2;
locks; strict;
comment	@ * @;


1.2
date	94.04.11.00.35.56;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.03.22.18.33.08;	author vandys;	state Exp;
branches;
next	;


desc
@Joystick driver test program
@


1.2
log
@Fix warnings
@
text
@#include <stdio.h>
#include <std.h>
#include "libjoystick.h"

void main(void)
{
  int i;
  uchar  btns, obtns = 0;
  ushort a = 0, oa, b = 0, ob, c = 0, oc, d = 0, od;

  if (joystick_connect() == -1) {
    fprintf(stderr,"Unable to connect to joystick\n");
    exit(1);
  }

  for (i = 0; i < 100; i++) {
    oa = a;
    ob = b;
    oc = c;
    od = d;
    obtns = btns;
    joystick_read(&a, &b, &c, &d, &btns);
    printf("%04x %04x %04x %04x %02x\n", a, b, c, d, btns);
  }

  if (joystick_disconnect() == -1) {
    fprintf(stderr,"Unable to disconnect from joystick\n");
    exit(1);
  }
}
@


1.1
log
@Initial revision
@
text
@d2 1
@
