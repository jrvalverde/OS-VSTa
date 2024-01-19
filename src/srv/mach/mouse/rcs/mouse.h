head	1.5;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.5
date	94.06.21.20.57.42;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.05.30.04.05.17;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.09.03.32.09;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.02.28.22.03.02;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.30.12;	author vandys;	state Exp;
branches;
next	;


desc
@General mouse defs
@


1.5
log
@Convert to openlog()
@
text
@/* 
 * mouse.h
 *    Data and function prototypes for a fairly universal mouse driver.
 *
 * Copyright (C) 1993 by G.T.Nicol, all rights reserved.
 */

#ifndef __VSTA_MOUSE_H__
#define __VSTA_MOUSE_H__

#include <sys/types.h>
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>

#include "nec_bus.h"
#include "ms_bus.h"
#include "logi_bus.h"
#include "ibmrs232.h"
#include "ps2aux.h"

/*
 * Mouse button definitions
 */
#define MOUSE_LEFT_BUTTON    (1 << 0)
#define MOUSE_RIGHT_BUTTON   (1 << 1)
#define MOUSE_MIDDLE_BUTTON  (1 << 2)

/*
 * An open file
 */
struct file {
  uint f_gen;   /* Generation of access */
  uint f_flags; /* User access bits */
};

/*
 * Device specific mouse handling functions 
 */
typedef struct {
   void (*mouse_poller_entry_point)(void);
   void (*mouse_interrupt)(void);
   void (*mouse_coordinates)(ushort x, ushort y);
   void (*mouse_bounds)(ushort x1, ushort y1, ushort x2, ushort y2);
   void (*mouse_update_period)(ushort period);
} mouse_function_table;

/*
 * Pointer related mouse data
 */
typedef struct {
   uchar          buttons;             /* current button state */
   ushort         x;                   /* current X coordinate */
   ushort         y;                   /* current Y coordinate */
   ushort         bx1,by1,bx2,by2;     /* bounding box for the mouse */
} mouse_pointer_data_t;

/*
 * Overall mouse data
 */
typedef struct {
   mouse_function_table  functions;
   mouse_pointer_data_t  pointer_data;
   int                   irq_number;
   int                   update_frequency;
   char                  enable_interrupts;
   int                   type_id;
} mouse_data_t;

/* main.c */
extern struct prot mouse_prot;
extern port_t      mouse_port;  /* Port we receive contacts through */
extern port_name   mouse_name;  /* Name for out port */
extern uint        mouse_accgen;

/* mouse.c */
extern mouse_data_t  mouse_data;
extern void          mouse_initialise(int argc, char **argv);
extern void          mouse_read(struct msg *m, struct file *f);
extern void          mouse_write(struct msg *m, struct file *f);

/* stat.c */
extern void mouse_stat(struct msg *m, struct file *f);
extern void mouse_wstat(struct msg *m, struct file *f);

#endif /* __VSTA_MOUSE_H__ */
@


1.4
log
@Add PS2, convert to syslog, convert to RS-232 server
@
text
@a74 1
extern char	   mouse_sysmsg[];
@


1.3
log
@Clean up white space
@
text
@d20 1
d23 7
d75 1
@


1.2
log
@Don't need ports.h any more
@
text
@a78 1

@


1.1
log
@Initial revision
@
text
@a14 1
#include <sys/ports.h>
@
