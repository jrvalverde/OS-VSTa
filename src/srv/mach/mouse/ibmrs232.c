/*
 * ibmrs232.c
 *    Polling driver for the IBM serial mouse.
 *
 * Original code copyright (C) 1993 by G.T.Nicol, all rights reserved.
 * Modified by Dave Hudson to use the rs232 server to transfer data via
 * the serial port
 *
 * A lot of this code came from the Linux/MGR mouse driver code.
 */

#include <fdl.h>
#include <fcntl.h>
#include <stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <mach/io.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/fs.h>
#include "machine.h"
#include "mouse.h"

/*
 * Our function table
 */
static mouse_function_table ibm_serial_functions = {
   ibm_serial_poller_entry_point,	/* mouse_poller_entry_point */
   NULL,				/* mouse_interrupt */
   ibm_serial_coordinates,		/* mouse_coordinates */
   ibm_serial_bounds,			/* mouse_bounds */
   ibm_serial_update_period,		/* mouse_update_period */
};

/*
 * Each type of serial mouse records its details in one of these
 */
struct ibm_serial_config {
   int mask0;
   int test0;
   int mask1;
   int test1;
   int dmin;
   int dmax;
   int baud;
   int databits;
   int stopbits;
   char parity[5];
};

/*
 * General data used by the driver
 */
static ushort      ibm_serial_delay_period = 100;
static uint        ibm_serial_update_allowed = TRUE;
static ibm_model_t ibm_serial_model = RS_MICROSOFT;
static port_t	   ibm_serial_port;
static uchar *	   ibm_serial_buffer;
static int	   ibm_serial_bufoffs = 0;

static struct ibm_serial_config ibm_serial_data[5] = {
   {0x40, 0x40, 0x40, 0x00,		/* MicroSoft */
    3, 4, 1200, 7, 1, "none"},
   {0xf8, 0x80, 0x00, 0x00,		/* MouseSystems 3 */
    3, 3, 1200, 8, 2, "none"},
   {0xf8, 0x80, 0x00, 0x00,		/* MouseSystems 5 */
    5, 5, 1200, 8, 2, "none"},
   {0xe0, 0x80, 0x80, 0x00,		/* MMSeries */
    3, 3, 1200, 8, 1, "odd"},
   {0xe0, 0x80, 0x80, 0x00,		/* Logitech */
    3, 3, 1200, 8, 1, "odd"}
};

/*
 * rs232_putc()
 *    Send a character to the rs232 server
 *
 * We communicate direct with the server as this eliminates a little of the
 * overhead in the libc code, but mainly because the rs232 server is a "tty"
 * server and we don't want our I/O tty'd
 */
static void
rs232_putc(uchar c)
{
   struct msg m;
   
   m.m_op = FS_WRITE;
   m.m_buf = &c;
   m.m_buflen = 1;
   m.m_arg = 1;
   m.m_arg1 = 0;
   m.m_nseg = 1;

   msg_send(ibm_serial_port, &m);
}

/*
 * rs232_read()
 *    Read all of the characters currently in the rs232's buffer
 *
 * Returns the number of characters read.
 */
static int
rs232_read(void)
{
   struct msg m;
   int x;
   
   m.m_op = M_READ|FS_READ;
   m.m_buf = &ibm_serial_buffer[ibm_serial_bufoffs];
   m.m_arg = 0;
   m.m_buflen = IBM_SERIAL_BUFSIZ - ibm_serial_bufoffs;
   m.m_nseg = 1;
   m.m_arg1 = 0;
   
   x = msg_send(ibm_serial_port, &m);
   if (x > 0) {
      ibm_serial_bufoffs += x;
   }
   return x;
}

/*
 * ibm_serial_check_status()
 *    Read in the mouse coordinates.
 */
static void
ibm_serial_check_status(void)
{
   short new_x, new_y;
   char x_off, y_off;
   int i = 0, x, buttons, got_packet;
   struct ibm_serial_config *m_data = &ibm_serial_data[ibm_serial_model];
   uchar *buffer;
   static btn_cvt[8] = {
      0,
      MOUSE_RIGHT_BUTTON,
      MOUSE_MIDDLE_BUTTON,
      MOUSE_RIGHT_BUTTON | MOUSE_MIDDLE_BUTTON,
      MOUSE_LEFT_BUTTON,
      MOUSE_LEFT_BUTTON | MOUSE_RIGHT_BUTTON,
      MOUSE_LEFT_BUTTON | MOUSE_MIDDLE_BUTTON,
      MOUSE_LEFT_BUTTON | MOUSE_RIGHT_BUTTON | MOUSE_MIDDLE_BUTTON
   };

   set_semaphore(&ibm_serial_update_allowed, FALSE);

   new_x = mouse_data.pointer_data.x;
   new_y = mouse_data.pointer_data.y;

   rs232_read();

restart:
   /*
    * Find a header byte
    */
   while ((i < ibm_serial_bufoffs)
           && ((ibm_serial_buffer[i] & m_data->mask0) != m_data->test0)) {
      i++;
   }

   /*
    * read in the rest of the packet
    */
   while (ibm_serial_bufoffs - i >= m_data->dmax) {
      buffer = &ibm_serial_buffer[i];
      i++;

      x = 1;
      got_packet = 0;
      while(!got_packet && x < m_data->dmax) {
         /*
          * Check whether we have a data packet
          */
         if ((buffer[x] & m_data->mask1) != m_data->test1
             || buffer[x] == 0x80) {
	    if (x >= m_data->dmin) {
	       /*
	        * If we've got a complete "minimum" packet let's use it
	        */
	       got_packet = 1;
	    } else {
	       /*
	        * If we don't have a complete "minimum" packet go back
	        * and start processing from this new starting point
	        */
	       goto restart;
	    }
         } else {
	    /*
	     * We've found another byte in the middle of a packet, so let's
	     * carry on
	     */
            i++;
            x++;
         }
      }

      switch (ibm_serial_model) {
      case RS_MICROSOFT:	/* Microsoft */
      default:
         buttons = ((buffer[0] & 0x20) >> 5)
         	   | ((buffer[0] & 0x10) >> 3)
         	   | ((x > m_data->dmin) ? ((buffer[3] & 0x20) >> 3) : 0);
         x_off = (char) (((buffer[0] & 0x03) << 6) | (buffer[1] & 0x3f));
         y_off = (char) (((buffer[0] & 0x0c) << 4) | (buffer[2] & 0x3f));
         break;
      case RS_MOUSE_SYS_3:	/* Mouse Systems 3 byte */
         buttons = btn_cvt[(~buffer[0]) & 0x07];
         x_off = (char) (buffer[1]);
         y_off = -(char) (buffer[2]);
         break;
      case RS_MOUSE_SYS_5:	/* Mouse Systems Corp 5 bytes */
         buttons = btn_cvt[(~buffer[0]) & 0x07];
         x_off = (char) (buffer[1]) + (char) (buffer[3]);
         y_off = -((char) (buffer[2]) + (char) (buffer[4]));
         break;
      case RS_MM:		/* MM Series */
      case RS_LOGITECH:		/* Logitech */
         buttons = btn_cvt[buffer[0] & 0x07];
         x_off = (buffer[0] & 0x10) ? buffer[1] : -buffer[1];
         y_off = (buffer[0] & 0x08) ? -buffer[2] : buffer[2];
         break;
      }

      /*
       *  If they've changed, update  the current coordinates
       */
      if (x_off != 0 || y_off != 0) {
         new_x += x_off;
         new_y += y_off;
         /*
          * Make sure we honour the bounding box
          */
         if (new_x < mouse_data.pointer_data.bx1) {
	    new_x = mouse_data.pointer_data.bx1;
	 }
         if (new_x > mouse_data.pointer_data.bx2) {
	    new_x = mouse_data.pointer_data.bx2;
	 }
         if (new_y < mouse_data.pointer_data.by1) {
	    new_y = mouse_data.pointer_data.by1;
         }
         if (new_y > mouse_data.pointer_data.by2) {
	    new_y = mouse_data.pointer_data.by2;
	 }
         /*
          *  Set up the new mouse position
          */
         mouse_data.pointer_data.x = new_x;
         mouse_data.pointer_data.y = new_y;
      }
      mouse_data.pointer_data.buttons = buttons;
   }

   /*
    * After all that, do we need to shunt our buffers about?
    */
   if (i) {
      if (ibm_serial_bufoffs - i) {
         memmove(ibm_serial_buffer, &ibm_serial_buffer[i],
         	 ibm_serial_bufoffs - i);
      }
      ibm_serial_bufoffs -= i;
   }

   set_semaphore(&ibm_serial_update_allowed, TRUE);
}

/*
 * ibm_serial_poller_entry_point()
 *    Main loop of the polling thread.
 */
void
ibm_serial_poller_entry_point(void)
{
   for (;;) {
      ibm_serial_check_status();
      if (ibm_serial_delay_period) {
	 __msleep(ibm_serial_delay_period);
      }
   }
}

/*
 * ibm_serial_bounds()
 *    Change or read the mouse bounding box.
 */
void
ibm_serial_bounds(ushort x1, ushort y1, ushort x2, ushort y2)
{
   while (!ibm_serial_update_allowed)
   	__msleep(1);
   set_semaphore(&ibm_serial_update_allowed, FALSE);
   mouse_data.pointer_data.bx1 = x1;
   mouse_data.pointer_data.bx2 = x2;
   mouse_data.pointer_data.by1 = y1;
   mouse_data.pointer_data.by2 = y2;
   set_semaphore(&ibm_serial_update_allowed, TRUE);
}

/*
 * ibm_serial_coordinates()
 *    Change or read the mouse coordinates.
 */
void
ibm_serial_coordinates(ushort x, ushort y)
{
   while (!ibm_serial_update_allowed) {
   	__msleep(1);
   }
   set_semaphore(&ibm_serial_update_allowed, FALSE);
   mouse_data.pointer_data.x = x;
   mouse_data.pointer_data.y = y;
   set_semaphore(&ibm_serial_update_allowed, TRUE);
}

/*
 * ibm_serial_update_period()
 *    Read the mouse button status
 */
void
ibm_serial_update_period(ushort period)
{
   if (period == 0)
      period = 5;
   set_semaphore(&ibm_serial_update_allowed, FALSE);
   ibm_serial_delay_period = 1000 / period;
   mouse_data.update_frequency = period;
   set_semaphore(&ibm_serial_update_allowed, TRUE);

   if (ibm_serial_model == RS_LOGITECH) {
      rs232_putc('S');
   }
   if (period <= 0)
      rs232_putc('O');
   else if (period <= 15)
      rs232_putc('J');
   else if (period <= 27)
      rs232_putc('K');
   else if (period <= 42)
      rs232_putc('L');
   else if (period <= 60)
      rs232_putc('R');
   else if (period <= 85)
      rs232_putc('M');
   else if (period <= 125)
      rs232_putc('Q');
   else
      rs232_putc('N');
}

/*
 * ibm_serial_usage()
 *    Inform the user how to use the serial mouse driver
 */
static void
ibm_serial_usage(void)
{
   fprintf(stderr,
	   "usage: mouse -type serial <-p port | rs232-dev-file> " \
	   "[-delay=<delay-in-ms>]\n" \
	   "       <-microsoft | -mouse_sys_3 | -mouse_sys_5 |" \
	   " -mm | -logitech>\n");
   exit(1);
}

/*
 * ibm_serial_parse_args()
 *    Parse the command line.
 *
 * We also establish a connection to the relevant rs232 server port.
 */
static void
ibm_serial_parse_args(int argc, char **argv)
{
   int microsoft_option = 0, mouse_sys_3_option = 0, mouse_sys_5_option = 0;
   int mm_option = 0, logitech_option = 0;
   int arg, arg_st;
   char st_msg[32];
   struct ibm_serial_config *mcfg;

   /*
    * Check we have enough parameters
    */
   if (argc < 5) {
      ibm_serial_usage();
   }

   /*
    * Look for the rs232 server details
    */
   if (strcmp(argv[3], "-p") == 0) {
      /*
       * Looks like we're attaching straight to the server
       */
      if (!argv[4]) {
         fprintf(stderr, "mouse: missing port parameter\n");
         exit(1);
      }
      ibm_serial_port = path_open(argv[4], ACC_READ|ACC_WRITE|ACC_CHMOD);
      if (ibm_serial_port < 0) {
	 syslog(LOG_ERR, "unable to get connection to '%s'", argv[4]);
	 exit(1);
      }
      arg_st = 5;
   } else {
      /*
       * Try to attach to the server via the specified mounted file
       */
      int fd;

      fd = open(argv[3], O_RDWR);
      if (fd < 0) {
	 syslog(LOG_ERR, "unable to open path to '%s'", argv[3]);
	 exit(1);
      }
      ibm_serial_port = __fd_port(fd);
      arg_st = 4;
   }

   /*
    * Parse the command line args - things such as establishing the mouse
    * type and the baud rate
    */
   for (arg = arg_st; arg < argc; arg++) {
      if (argv[arg][0] == '-') {
	 if (strcmp(&argv[arg][1], "delay=") == 0) {
	    if (argv[arg][7] == '\0')
	       ibm_serial_usage();
	    ibm_serial_delay_period = atoi(&argv[arg][7]);
	 } else if (strcmp(&argv[arg][1], "microsoft") == 0) {
	    microsoft_option = 1;
	    ibm_serial_model = RS_MICROSOFT;
	 } else if (strcmp(&argv[arg][1], "mouse_sys_3") == 0) {
	    mouse_sys_3_option = 1;
	    ibm_serial_model = RS_MOUSE_SYS_3;
	 } else if (strcmp(&argv[arg][1], "mouse_sys_5") == 0) {
	    mouse_sys_5_option = 1;
	    ibm_serial_model = RS_MOUSE_SYS_5;
	 } else if (strcmp(&argv[arg][1], "mm") == 0) {
	    mm_option = 1;
	    ibm_serial_model = RS_MM;
	 } else if (strcmp(&argv[arg][1], "logitech") == 0) {
	    logitech_option = 1;
	    ibm_serial_model = RS_LOGITECH;
	 } else {
	    fprintf(stderr, "mouse: unknown option 's' - aborting\n",
	    	    argv[arg]);
	    exit(1);
	 }
      }
   }

   mcfg = &ibm_serial_data[ibm_serial_model];

   /*
    * Set DTR and RTS toggle to power up the mouse and reset it.  Also
    * establish the baud rate and the data/stop bit/parity settings.
    */
   sprintf(st_msg, "baud=%d\n", mcfg->baud);
   wstat(ibm_serial_port, st_msg);
   sprintf(st_msg, "databits=%d\n", mcfg->databits);
   wstat(ibm_serial_port, st_msg);
   sprintf(st_msg, "stopbits=%d\n", mcfg->stopbits);
   wstat(ibm_serial_port, st_msg);
   sprintf(st_msg, "parity=%s\n", mcfg->parity);
   wstat(ibm_serial_port, st_msg);

   wstat(ibm_serial_port, "dtr=1\n");
   wstat(ibm_serial_port, "rts=0\n");
   __msleep(120);
   wstat(ibm_serial_port, "rts=1\n");

   if (microsoft_option + mouse_sys_3_option + mouse_sys_5_option +
       mm_option + logitech_option > 1) {
      ibm_serial_usage();
   }
}


/*
 * ibm_serial_initialise()
 *    Initialise the mouse system.
 */
int
ibm_serial_initialise(int argc, char **argv)
{
   /*
    *  Initialise the system data.
    */
   mouse_data.functions            = ibm_serial_functions;
   mouse_data.pointer_data.x       = 320;
   mouse_data.pointer_data.y       = 320;
   mouse_data.pointer_data.buttons = 0;
   mouse_data.pointer_data.bx1     = 0;
   mouse_data.pointer_data.by1     = 0;
   mouse_data.pointer_data.bx2     = 639;
   mouse_data.pointer_data.by2     = 399;
   mouse_data.irq_number           = 0;
   mouse_data.update_frequency     = ibm_serial_delay_period;

   ibm_serial_buffer = (uchar *)malloc(IBM_SERIAL_BUFSIZ);
   if (!ibm_serial_buffer) {
      syslog(LOG_ERR, "unable to allocate data buffer");
      exit(1);
   }

   ibm_serial_parse_args(argc, argv);

   /*
    *  Make doubly sure that we have every ready for polling.
    */
   mouse_data.functions.mouse_interrupt = NULL;
   mouse_data.enable_interrupts = FALSE;
   ibm_serial_update_period(mouse_data.update_frequency);

   syslog(LOG_INFO, "serial mouse installed");

   return (0);
}
