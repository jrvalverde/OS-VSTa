head	1.4;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.4
date	94.06.21.20.57.42;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.05.30.04.04.43;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.04.09.03.32.09;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.30.12;	author vandys;	state Exp;
branches;
next	;


desc
@Microsoft bus mouse driver
@


1.4
log
@Convert to openlog()
@
text
@/* 
 * ms_bus.c
 *    Interrupt driven driver for the NEC bus mouse.
 *
 * Copyright (C) 1993 by G.T.Nicol, all rights reserved.
 *
 * A lot of this code is just taken from the Linux source.
 */

#include <stdio.h>
#include <std.h>
#include <syslog.h>
#include <time.h>
#include <mach/io.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include "mouse.h"

/*
 * Our function table
 */
static mouse_function_table ms_bus_functions = {
   NULL,                               /* mouse_poller_entry_point */
   ms_bus_interrupt,                   /* mouse_interrupt          */
   ms_bus_coordinates,                 /* mouse_coordinates        */
   ms_bus_bounds,                      /* mouse_bounds             */
   NULL,                               /* mouse_update_period      */
};

/*
 * ms_bus_interrupt()
 *    Handle a mouse interrupt.
 */
void
ms_bus_interrupt(void)
{
   short new_x, new_y;
   uchar dx, dy, buttons;
   
   new_x = mouse_data.pointer_data.x;
   new_y = mouse_data.pointer_data.y;

   outportb(MS_BUS_CONTROL_PORT, MS_BUS_COMMAND_MODE);
   outportb(MS_BUS_DATA_PORT, (inportb(MS_BUS_DATA_PORT) | 0x20));
   
   outportb(MS_BUS_CONTROL_PORT, MS_BUS_READ_X);
   dx = inportb(MS_BUS_DATA_PORT);
   
   outportb(MS_BUS_CONTROL_PORT, MS_BUS_READ_Y);
   dy = inportb(MS_BUS_DATA_PORT);
   
   outportb(MS_BUS_CONTROL_PORT, MS_BUS_READ_BUTTONS);
   buttons = ~(inportb(MS_BUS_DATA_PORT)) & 0x07;
   
   outportb(MS_BUS_CONTROL_PORT, MS_BUS_COMMAND_MODE);
   outportb(MS_BUS_DATA_PORT, (inportb(MS_BUS_DATA_PORT) & 0xdf));

   /*
    *  If they've changed, update  the current coordinates
    */
   if (dx != 0 || dy != 0) {
      new_x += dx;
      new_y += dy;
      /*
       *  Make sure we honour the bounding box
       */
      if (new_x < mouse_data.pointer_data.bx1)
         new_x = mouse_data.pointer_data.bx1;
      if (new_x > mouse_data.pointer_data.bx2)
         new_x = mouse_data.pointer_data.bx2;
      if (new_y < mouse_data.pointer_data.by1)
         new_y = mouse_data.pointer_data.by1;
      if (new_y > mouse_data.pointer_data.by2)
         new_y = mouse_data.pointer_data.by2;
      /*
       *  Set up the new mouse position
       */
      mouse_data.pointer_data.x = new_x;
      mouse_data.pointer_data.y = new_y;
   }

   /*
    * Not sure about this... but I assume that the MS mouse is the
    * same as the NEC one.
    */
   switch (buttons) {                 /* simulate a 3 button mouse here */
   case 4:
      mouse_data.pointer_data.buttons = MOUSE_LEFT_BUTTON;      /* left   */
      break;
   case 1:
      mouse_data.pointer_data.buttons = MOUSE_RIGHT_BUTTON;     /* right  */
      break;
   case 0:
      mouse_data.pointer_data.buttons = MOUSE_MIDDLE_BUTTON;    /* middle */
      break;
   default:
      mouse_data.pointer_data.buttons = 0;                      /* none   */
   };
}


/*
 * ms_bus_bounds()
 *    Change or read the mouse bounding box.
 */
void
ms_bus_bounds(ushort x1, ushort y1, ushort x2, ushort y2)
{
   mouse_data.pointer_data.bx1 = x1;
   mouse_data.pointer_data.bx2 = x2;
   mouse_data.pointer_data.by1 = y1;
   mouse_data.pointer_data.by2 = y2;
}

/*
 * ms_bus_coordinates()
 *    Change or read the mouse coordinates.
 */
void
ms_bus_coordinates(ushort x, ushort y)
{
   mouse_data.pointer_data.x = x;
   mouse_data.pointer_data.y = y;
}

/*
 * ms_bus_initialise()
 *    Initialise the mouse system.
 */
int
ms_bus_initialise(int argc, char **argv)
{
   int mouse_seen = FALSE;
   int loop,dummy;

   /*
    *  Initialise the system data.
    */
   mouse_data.functions             = ms_bus_functions;
   mouse_data.pointer_data.x        = 320;
   mouse_data.pointer_data.y        = 320;
   mouse_data.pointer_data.buttons  = 0;
   mouse_data.pointer_data.bx1      = 0;
   mouse_data.pointer_data.by1      = 0;
   mouse_data.pointer_data.bx2      = 639;
   mouse_data.pointer_data.by2      = 399;
   mouse_data.irq_number            = MS_BUS_IRQ;
   mouse_data.update_frequency      = 0;

   /*
    * Parse our args...
    */
   for(loop=1; loop<argc; loop++){
      if(strcmp(argv[loop],"-x_size") == 0){
	 if(++loop == argc){
	    syslog(LOG_ERR, "bad -x_size parameter");
	    break;
	 }
	 mouse_data.pointer_data.x   = atoi(argv[loop])/2;
	 mouse_data.pointer_data.bx2 = atoi(argv[loop]);
      }
      if(strcmp(argv[loop],"-y_size") == 0){
	 if(++loop == argc){
	    syslog(LOG_ERR, "bad -y_size parameter");
	    break;
	 }
	 mouse_data.pointer_data.y   = atoi(argv[loop])/2;
	 mouse_data.pointer_data.by2 = atoi(argv[loop]);
      }
   }

   /*
    * Get our hardware ports.
    */
   if (enable_io(MICROSOFT_LOW_PORT, MICROSOFT_HIGH_PORT) < 0) {
      syslog(LOG_ERR, "unable to enable I/O ports for mouse");
      return (-1);
   }

   /*
    * Check for the mouse.
    */
   if (inportb(MS_BUS_ID_PORT) == 0xde) {
      __msleep(100);
      dummy = inportb(MS_BUS_ID_PORT);
      for (loop = 0; loop < 4; loop++) {
	 __msleep(100);
	 if (inportb(MS_BUS_ID_PORT) == 0xde) {
	    __msleep(100);
	    if (inportb(MS_BUS_ID_PORT) == dummy) {
	       mouse_seen = TRUE;
	    } else {
	       mouse_seen = FALSE;
	    }
	 } else
	    mouse_seen = FALSE;
      }
   }

   if (mouse_seen == FALSE) {
      return(-1);
   }

   mouse_data.functions.mouse_poller_entry_point = NULL;
   mouse_data.enable_interrupts                  = TRUE;

   /*
    * Enable mouse and go!
    */
   outportb(MS_BUS_CONTROL_PORT, MS_BUS_START);
   MS_BUS_INT_ON();

   syslog(LOG_INFO, "Microsoft bus mouse detected and installed");
   return (0);
}
@


1.3
log
@Add PS2, convert to syslog, convert to RS-232 server
@
text
@d156 1
a156 1
	    syslog(LOG_ERR, "%s bad -x_size parameter", mouse_sysmsg);
d164 1
a164 1
	    syslog(LOG_ERR, "%s bad -y_size parameter", mouse_sysmsg);
d176 1
a176 1
      syslog(LOG_ERR, "%s unable to enable I/O ports for mouse", mouse_sysmsg);
d213 1
a213 2
   syslog(LOG_INFO, "%s Microsoft bus mouse detected and installed",
          mouse_sysmsg);
@


1.2
log
@Clean up white space
@
text
@d12 3
d16 1
a16 1
#include "machine.h"
d88 1
a88 1
      mouse_data.pointer_data.buttons = (1 << 1);       /* left   */
d91 1
a91 1
      mouse_data.pointer_data.buttons = (1 << 3);       /* right  */
d94 1
a94 1
      mouse_data.pointer_data.buttons = (1 << 2);       /* middle */
d97 1
a97 1
      mouse_data.pointer_data.buttons = 0;              /* none   */
d156 1
a156 1
	    fprintf(stderr,"Mouse: bad -x_size parameter.\n");
d164 1
a164 1
	    fprintf(stderr,"Mouse: bad -y_size parameter.\n");
d176 1
a176 1
      fprintf(stderr, "Mouse: Unable to enable I/O ports for mouse.\n");
d213 2
a214 1
   printf("Microsoft Bus mouse detected and installed.\n");
@


1.1
log
@Initial revision
@
text
@a212 4




@
