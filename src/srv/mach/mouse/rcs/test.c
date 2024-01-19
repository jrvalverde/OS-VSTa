head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2;
locks; strict;
comment	@ * @;


1.3
date	94.04.09.03.32.09;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.01.13.06.33.14;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.30.52;	author vandys;	state Exp;
branches;
next	;


desc
@Test program
@


1.3
log
@Clean up white space
@
text
@#include <stdio.h>
#include "libmouse.h"

int 
main(void)
{
   uchar  buttons, obuttons;
   ushort x, y, ox, oy;

   if (mouse_connect() == -1){
      fprintf(stderr,"Unable to connect to mouse\n");
      exit(1);
   }

   for (;;) {
      mouse_get_buttons(&buttons);
      mouse_get_coordinates(&x, &y);
      if ((x != ox) || (y != oy) || (buttons != obuttons)) {
	      printf("%d %d %d %3d %3d\n",
		     (buttons & MOUSE_LEFT_BUTTON) > 0,
		     (buttons & MOUSE_MIDDLE_BUTTON) > 0,
		     (buttons & MOUSE_RIGHT_BUTTON) > 0,
		     x, y);
     	     ox = x;
	     oy = y;
	     obuttons = buttons;
     } else {
     	__msleep(200);
     }
   }
}
@


1.2
log
@Bring up microsoft RS-232 mouse
@
text
@a31 4




@


1.1
log
@Initial revision
@
text
@d7 2
a8 2
   uchar  buttons;
   ushort x,y;
d10 1
a10 1
   if(mouse_connect() == -1){
d15 1
a15 1
   while(1){
d17 13
a29 6
      mouse_get_coordinates(&x,&y);
      printf("%d %d %d %3d %3d\n",
	     (buttons & MOUSE_LEFT_BUTTON) > 0,
	     (buttons & MOUSE_MIDDLE_BUTTON) > 0,
	     (buttons & MOUSE_RIGHT_BUTTON) > 0,
	     x,y);
@
