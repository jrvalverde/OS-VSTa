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
date	94.05.30.04.04.43;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.09.03.31.58;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.02.28.22.03.12;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.30.12;	author vandys;	state Exp;
branches;
next	;


desc
@Main server handling
@


1.5
log
@Convert to openlog()
@
text
@/* 
 * main.c 
 *    The main loop, and server entry point.
 *
 * Copyright (C) 1993 by G.T.Nicol, all rights reserved.
 *
 * This code is based on the server code found in cons, wd, etc.
 */

#include <sys/perm.h>
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/assert.h>
#include <sys/namer.h>
#include <sys/syscall.h>
#include <hash.h>
#include <std.h>
#include <syslog.h>
#include <stdio.h>

#include "mouse.h"

struct hash *mouse_hash;                 /* Map session->context structure   */
port_t       mouse_port;                 /* Port we receive contacts through */
port_name    mouse_name;                 /* Name for out port                */
uint         mouse_accgen;               /* generation access count          */
struct prot  mouse_prot = {              /* mouse protection                 */
   1,
   ACC_READ | ACC_WRITE,
   {1},
   {ACC_CHMOD}
};

/*
 * mouse_new_client()
 *     Create new per-connect structure.
 */
static void
mouse_new_client(struct msg * m)
{
   struct file *f;
   struct perm *perms;
   int uperms, nperms, desired;

   /*
    *  See if they're OK to access
    */
   perms = (struct perm *) m->m_buf;
   nperms = (m->m_buflen) / sizeof(struct perm);
   uperms = perm_calc(perms, nperms, &mouse_prot);
   desired = m->m_arg & (ACC_WRITE | ACC_READ | ACC_CHMOD);
   if ((uperms & desired) != desired) {
      printf("no permission\n");
      msg_err(m->m_sender, EPERM);
      return;
   }
   /*
    *  Get data structure
    */
   if ((f = malloc(sizeof(struct file))) == 0) {
      msg_err(m->m_sender, strerror());
      return;
   }
   /*
    *  Fill in fields.
    */
   f->f_gen = mouse_accgen;
   f->f_flags = uperms;

   /*
    *  Hash under the sender's handle
    */
   if (hash_insert(mouse_hash, m->m_sender, f)) {
      free(f);
      msg_err(m->m_sender, ENOMEM);
      return;
   }
   /*
    *  Return acceptance
    */
   msg_accept(m->m_sender);
}

/*
 * mouse_dup_client()
 *     Duplicate current file access onto new session
 */
static void
mouse_dup_client(struct msg * m, struct file * fold)
{
   struct file *f;

   /*
    *  Get data structure
    */
   if ((f = malloc(sizeof(struct file))) == 0) {
      msg_err(m->m_sender, strerror());
      return;
   }
   /*
    *  Fill in fields.  Simply duplicate old file.
    */
   *f = *fold;

   /*
    *  Hash under the sender's handle
    */
   if (hash_insert(mouse_hash, m->m_arg, f)) {
      free(f);
      msg_err(m->m_sender, ENOMEM);
      return;
   }
   /*
    *  Return acceptance
    */
   m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
   msg_reply(m->m_sender, m);
}


/*
 * mouse_dead_client
 *     Someone has gone away.  Free their info.
 */
static void
mouse_dead_client(struct msg * m, struct file * f)
{
   (void) hash_delete(mouse_hash, m->m_sender);
   free(f);
}

/*
 * mouse_check_gen
 *     See if the mouse is still accessible to them
 */
static int
mouse_check_gen(struct msg * m, struct file * f)
{
   if (f->f_gen != mouse_accgen) {
      msg_err(m->m_sender, EIO);
      return (1);
   }
   return (0);
}

/*
 * mouse_main()
 *     Endless loop to receive and serve requests
 */
static void
mouse_main()
{
   int x;
   struct file *f;
   struct msg msg;

   char buffer[128];
   strcpy(buffer, "hello");

loop:
   /*
    *  Receive a message, log an error and then keep going
    */
   x = msg_receive(mouse_port, &msg);
   if (x < 0) {
      syslog(LOG_ERR, "msg_receive");
      goto loop;
   }
   /*
    *  Categorize by basic message operation
    */
   f = hash_lookup(mouse_hash, msg.m_sender);
   switch (msg.m_op) {
   case M_CONNECT:                       /* New client                    */
      mouse_new_client(&msg);
      break;
   case M_DISCONNECT:                    /* Client done                   */
      mouse_dead_client(&msg, f);
      break;
   case M_DUP:                           /* File handle dup during exec() */
      mouse_dup_client(&msg, f);
      break;
   case M_ABORT:                         /* Aborted operation             */
      msg_reply(msg.m_sender, &msg);
      break;
   case M_ISR:                           /* Interrupt                      */
      if (mouse_data.functions.mouse_interrupt != NULL) {
         (*mouse_data.functions.mouse_interrupt) ();
      }
      break;
   case FS_STAT:                         /* Stat of file                   */
      if (mouse_check_gen(&msg, f)) {
         break;
      }
      mouse_stat(&msg, f);
      break;
   case FS_WSTAT:                        /* Write selected stat fields     */
      if (mouse_check_gen(&msg, f)) {
         break;
      }
      mouse_wstat(&msg, f);
      break;
   case FS_WRITE:
      if (mouse_check_gen(&msg, f)) {
         break;
      }
      mouse_write(&msg, f);
      break;
   case FS_READ:
      if (mouse_check_gen(&msg, f)) {
         break;
      }
      mouse_read(&msg, f);
      break;
   default:                              /* Unknown                        */
      msg_err(msg.m_sender, EINVAL);
      break;
   }
   goto loop;
}

/*
 * main()
 *    The main routine. Set up everything, then call the above.
 */
int
main(int argc, char **argv)
{
   /*
    * Initialize syslog
    */
   openlog("mouse", LOG_PID, LOG_DAEMON);

   /*
    *  Allocate handle->file hash table.
    */
   mouse_hash = hash_alloc(16);

   /*
    *  Initialise the mouse driver.
    */
   mouse_initialise(argc, argv);

   /*
    *  Get a port and name
    */
   mouse_port = msg_port((port_name) 0, &mouse_name);
   if (namer_register("srv/mouse", mouse_name) < 0) {
      syslog(LOG_ERR, "can't register name");
      exit(1);
   }

   /*
    *  Enable either polling or interrupts for updating the mouse data.
    */
   if (mouse_data.enable_interrupts) {
      if (enable_isr(mouse_port, mouse_data.irq_number)) {
         syslog(LOG_ERR, "unable to get IRQ line");
         exit(1);
      }
   } else {
      if (mouse_data.functions.mouse_poller_entry_point != NULL) {
         if (tfork(mouse_data.functions.mouse_poller_entry_point) == -1) {
           syslog(LOG_ERR, "unable to fork poller thread - exiting");
           exit(1);
         }
      } else {
         syslog(LOG_INFO, "no interrupt or polling installed");
      }
   }

   /*
    *  Go, and never come back
    */
   mouse_main();

   /* NOTREACHED */
   return (-1);
}
@


1.4
log
@Add PS2, convert to syslog, convert to RS-232 server
@
text
@a26 2
char	     mouse_sysmsg[]              /* Syslog message prefix            */
   = "mouse (srv/mouse)";
d166 1
a166 1
      syslog(LOG_ERR, "%s msg_receive", mouse_sysmsg);
d230 5
d249 1
a249 1
      syslog(LOG_ERR, "%s can't register name", mouse_sysmsg);
d258 1
a258 1
         syslog(LOG_ERR, "%s unable to get IRQ line", mouse_sysmsg);
d264 1
a264 2
           syslog(LOG_ERR, "%s unable to fork poller thread - exiting",
                  mouse_sysmsg);
d268 1
a268 2
         syslog(LOG_INFO, "%s no interrupt or polling installed",
                mouse_sysmsg);
@


1.3
log
@Clean up white space
@
text
@d14 2
d18 1
a18 1

d27 2
a141 1
      printf("bad accgen\n");
d168 1
a168 1
      perror("mouse: msg_receive");
d237 5
d246 1
a246 1
      printf("mouse: can't register name\n");
a248 4
   /*
    *  Initialise the mouse driver.
    */
   mouse_initialise(argc, argv);
d255 1
a255 1
         fprintf(stderr, "Mouse: Unable to get IRQ line\n");
d261 2
a262 1
           fprintf(stderr, "Mouse: Unable to for poller thread. Exiting...\n");
d266 2
a267 1
         fprintf(stderr, "Mouse: No interrupt or polling installed.\n");
@


1.2
log
@Remove obsolete debugging
@
text
@a272 2


@


1.1
log
@Initial revision
@
text
@a227 10
#ifdef DEBUG
   int scrn, kbd;

   kbd = msg_connect(PORT_KBD, ACC_READ);
   (void) __fd_alloc(kbd);
   scrn = msg_connect(PORT_CONS, ACC_WRITE);
   (void) __fd_alloc(scrn);
   (void) __fd_alloc(scrn);
#endif

@
