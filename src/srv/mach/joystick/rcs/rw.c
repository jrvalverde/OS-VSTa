head	1.4;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2;
locks; strict;
comment	@ * @;


1.4
date	94.06.21.20.58.08;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.05.30.21.27.25;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.04.11.00.35.56;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.03.22.18.33.08;	author vandys;	state Exp;
branches;
next	;


desc
@Read/write message handling
@


1.4
log
@Convert to openlog()
@
text
@/*
 * Filename:	rw.c
 * Author:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Started:	5th January 1994
 * Last Update: 10th May 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description: Deals with read/write requests to the joystick device
 */
#include <pio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/assert.h>
#include <sys/seg.h>
#include <sys/syscall.h>
#include <mach/io.h>
#include <mach/param.h>
#include <mach/pit.h>
#include "joystick.h"

extern uchar js_mask;
extern int js_channels;

/*
 * js_timer_read()
 *	Read the 8254 interval timer to determine ticks down to around
 * 	microsecond level
 *
 * It might be a good idea to move this to a library somewhere, but here
 * will do for now (especially during development)
 */
static ushort
js_timer_read(void)
{
	int t;

	outportb(PIT_CTRL, 0);	/* Latch the current timer count */
	t = inportb(PIT_CH0);
	t += (inportb(PIT_CH0) << 8);
	return(t);
}

/*
 * js_adjust_time()
 *	Adjust a joystick port timing
 *
 * When we've read a timing from a joystick port (A-D conversion) we have
 * to apply some correction to compenstate for the state of the timer when
 * we started timing
 */
static void
js_adjust_time(ushort *t_read, ushort t_start)
{
	int i;

	if (*t_read != JS_NONE && *t_read != JS_TIMEOUT) {
		i = ((int)t_start - (int)*t_read);
		if (i < 0) {
			i += (PIT_TICK / HZ);
		}
		*t_read = (ushort)i;
	}
}

/*
 * js_read()
 *	Post a read to the joystick
 *
 * Note that we can't be all that clever here because PC joysticks are
 * pretty badly designed - the A-D conversion is done by timing the discharge
 * of voltages (set up by the joystick pots) through an RC network.  The
 * position of the joystick can be calculated from the amount of time taken
 * to get a match.  The one saving grace is that we can do all of this in
 * parallel across all of the joystick channels.
 *
 * When I eventually managed to find some logics for a joystick port it
 * appeared that there was a 22 ms time constant, but I haven't found the
 * range of voltages that the joystick can present to the interface yet.
 * Maybe it's time to dig out a DVM and have a look :-)
 */
void js_read(struct msg *m, struct file *f)
{
	static pio_buffer_t *buffer = NULL;
	struct time tm_start, tm_end;
	ushort jsd_ch_a, jsd_ch_b, jsd_ch_c, jsd_ch_d;
	uchar jsd_btns;
	int us_tstart;
	int now_time, last_time;
	uchar din;
	static uchar js_id = 'J';
	uchar time_wrap = 0;
	int tick_wrap = 0;

	/*
	 * Establish a PIO buffer if we don't have one already!
	 */
	if (buffer == NULL) {
		buffer = pio_create_buffer(NULL, 0);
	}

	/*
	 * Establish the default results
	 */
	jsd_ch_a = (js_mask & JS_CH_A) ? JS_TIMEOUT : JS_NONE;
	jsd_ch_b = (js_mask & JS_CH_B) ? JS_TIMEOUT : JS_NONE;
	jsd_ch_c = (js_mask & JS_CH_C) ? JS_TIMEOUT : JS_NONE;
	jsd_ch_d = (js_mask & JS_CH_D) ? JS_TIMEOUT : JS_NONE;

	/*
	* Find out the initial state of the timers
	*/
	time_get(&tm_start);
	us_tstart = js_timer_read();
	now_time = us_tstart;

	/*
	* Trigger the timers and wait for data pulses
	*/
	outportb(JS_DATA, 0xff);

	while((time_wrap < 2) && (din = inportb(JS_DATA) & js_mask)) {
		last_time = now_time;
		now_time = js_timer_read();
		if (din & JS_CH_A) {
			jsd_ch_a = now_time;
		}
		if (din & JS_CH_B) {
			jsd_ch_b = now_time;
		}
		if (din & JS_CH_C) {
			jsd_ch_c = now_time;
		}
		if (din & JS_CH_D) {
			jsd_ch_d = now_time;
		}

		/*
		 * Has the timer wrapped round?
		 */
		if (last_time < now_time) {
			time_wrap++;
		}
	}

	/*
	 * Now let's check that we have valid data, ie that we didn't
	 * get interrupted during the run or that we weren't timed out!
	 */
	time_get(&tm_end);
	tick_wrap = ((int)tm_end.t_usec - (int)tm_start.t_usec) /
		(1000000 / HZ);
	if (tick_wrap < 0) {
		tick_wrap += ((int)tm_end.t_sec - (int)tm_start.t_sec) * HZ;
	}

	if ((time_wrap < 2) && (tick_wrap < 2)) {
		/*
		 * If we've not timed out, compensate the results!
		 * Even if only one of these has timed out we can't trust
		 * any of the times - we don't know when the joystick was
		 * unplugged or when there was such an interruption that
		 * everything got out of sync :-(
		 */
		js_adjust_time(&jsd_ch_a, us_tstart);
		js_adjust_time(&jsd_ch_b, us_tstart);
		js_adjust_time(&jsd_ch_c, us_tstart);
		js_adjust_time(&jsd_ch_d, us_tstart);
	}

	/*
	 * Finally, check the button status
	 */
	jsd_btns = (inportb(JS_DATA) & (JS_BTN_MASK)) ^ (JS_BTN_MASK);

	/*
	 * We use PIO to handle the message structure
	 */
	pio_reset_buffer(buffer);
	pio_u_char(buffer, &js_id);
	pio_u_char(buffer, &jsd_btns);
	pio_short(buffer, (short *)&jsd_ch_a);
	pio_short(buffer, (short *)&jsd_ch_b);
	pio_short(buffer, (short *)&jsd_ch_c);
	pio_short(buffer, (short *)&jsd_ch_d);

	/*
	 * Then blat the data out :-)
	 */ 
	m->m_buf = buffer->buffer;
	m->m_buflen = buffer->buffer_pos;
	m->m_nseg = 1;
	m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * js_init()
 *	Initialise the joystick server and establish the number of
 *	joysticks connected
 */
void
js_init(void)
{
	char init_msg[32] = "";
	int j;

	/*
	 * Let's see if we can find any joysticks.  First we send 0xff to
	 * the port to trigger the one shots, then we pause for a while
	 * and finally we check to see if anything happened
	 */
	outportb(JS_DATA, 0xff);
	__usleep(JS_TIMEOUT);
	j = inportb(JS_DATA) & JS_CH_MASK;

	if (j == JS_CH_MASK) {
		syslog(LOG_ERR, "no device connections found");
		return;
	}
	if (!(j & JS_CH_A)) {
		strcat(init_msg, "A");
		js_mask |= JS_CH_A;
		js_channels++;
	}	
	if (!(j & JS_CH_B)) {
		if (js_mask) {
			strcat(init_msg, ", ");
		}
		strcat(init_msg, "B");
		js_mask |= JS_CH_B;
		js_channels++;
	}	
	if (!(j & JS_CH_C)) {
		if (js_mask) {
			strcat(init_msg, ", ");
		}
		strcat(init_msg, "C");
		js_mask |= JS_CH_C;
		js_channels++;
	}	
	if (!(j & JS_CH_D)) {
		if (js_mask) {
			strcat(init_msg, ", ");
		}
		strcat(init_msg, "D");
		js_mask |= JS_CH_D;
		js_channels++;
	}
	syslog(LOG_INFO, "connections found on channel%s %s\n", 
		(js_channels == 1) ? "" : "s", init_msg);
}
@


1.3
log
@Syslog support
@
text
@a9 2


a22 1

a24 2
extern char js_sysmsg[];

d34 2
a35 1
static ushort js_timer_read(void)
d37 1
a37 1
  int t;
d39 4
a42 5
  outportb(PIT_CTRL, 0);	/* Latch the current timer count */
  t = (int)inportb(PIT_CH0);
  t += (int)inportb(PIT_CH0) << 8;

  return t;
a44 1

d53 2
a54 1
static void js_adjust_time(ushort *t_read, ushort t_start)
d56 1
a56 1
  int i;
d58 7
a64 7
  if (*t_read != JS_NONE && *t_read != JS_TIMEOUT) {
    i = ((int)t_start - (int)*t_read);
    if (i < 0) {
      i += (PIT_TICK / HZ);
    }
    *t_read = (ushort)i;
  }
a66 1

d85 111
a195 110
  static pio_buffer_t *buffer = NULL;
  struct time tm_start, tm_end;
  ushort jsd_ch_a, jsd_ch_b, jsd_ch_c, jsd_ch_d;
  uchar jsd_btns;
  int us_tstart;
  int now_time, last_time;
  uchar din;
  static uchar js_id = 'J';
  uchar time_wrap = 0;
  int tick_wrap = 0;

  /*
   * Establish a PIO buffer if we don't have one already!
   */
  if (buffer == NULL) {
    buffer = pio_create_buffer(NULL, 0);
  }

  /*
   * Establish the default results
   */
  jsd_ch_a = (js_mask & JS_CH_A) ? JS_TIMEOUT : JS_NONE;
  jsd_ch_b = (js_mask & JS_CH_B) ? JS_TIMEOUT : JS_NONE;
  jsd_ch_c = (js_mask & JS_CH_C) ? JS_TIMEOUT : JS_NONE;
  jsd_ch_d = (js_mask & JS_CH_D) ? JS_TIMEOUT : JS_NONE;

  /*
   * Find out the initial state of the timers
   */
  time_get(&tm_start);
  us_tstart = js_timer_read();
  now_time = us_tstart;

  /*
   * Trigger the timers and wait for data pulses
   */
  outportb(JS_DATA, 0xff);
	
  while((time_wrap < 2) && (din = inportb(JS_DATA) & js_mask)) {
    last_time = now_time;
    now_time = js_timer_read();
    if (din & JS_CH_A) {
      jsd_ch_a = now_time;
    }
    if (din & JS_CH_B) {
      jsd_ch_b = now_time;
    }
    if (din & JS_CH_C) {
      jsd_ch_c = now_time;
    }
    if (din & JS_CH_D) {
      jsd_ch_d = now_time;
    }

    /*
     * Has the timer wrapped round?
     */
    if (last_time < now_time) {
      time_wrap++;
    }
  };

  /*
   * Now let's check that we have valid data, ie that we didn't
   * get interrupted during the run or that we weren't timed out!
   */
  time_get(&tm_end);
  tick_wrap = ((int)tm_end.t_usec - (int)tm_start.t_usec) / (1000000 / HZ);
  if (tick_wrap < 0) {
    tick_wrap += ((int)tm_end.t_sec - (int)tm_start.t_sec) * HZ;
  }

  if ((time_wrap < 2) && (tick_wrap < 2)) {
    /*
     * If we've not timed out, compensate the results!
     * Even if only one of these has timed out we can't trust
     * any of the times - we don't know when the joystick was
     * unplugged or when there was such an interruption that
     * everything got out of sync :-(
     */
    js_adjust_time(&jsd_ch_a, us_tstart);
    js_adjust_time(&jsd_ch_b, us_tstart);
    js_adjust_time(&jsd_ch_c, us_tstart);
    js_adjust_time(&jsd_ch_d, us_tstart);
  }

  /*
   * Finally, check the button status
   */
  jsd_btns = (inportb(JS_DATA) & (JS_BTN_MASK)) ^ (JS_BTN_MASK);

  /*
   * We use PIO to handle the message structure
   */
  pio_reset_buffer(buffer);
  pio_u_char(buffer, &js_id);
  pio_u_char(buffer, &jsd_btns);
  pio_short(buffer, (short *)&jsd_ch_a);
  pio_short(buffer, (short *)&jsd_ch_b);
  pio_short(buffer, (short *)&jsd_ch_c);
  pio_short(buffer, (short *)&jsd_ch_d);

  /*
   * Then blat the data out :-)
   */ 
  m->m_buf = buffer->buffer;
  m->m_buflen = buffer->buffer_pos;
  m->m_nseg = 1;
  m->m_arg = m->m_arg1 = 0;
  msg_reply(m->m_sender, m);
a197 1

d203 2
a204 1
void js_init(void)
d206 2
a207 2
  char init_msg[32] = "";
  int j;
d209 44
a252 44
  /*
   * Let's see if we can find any joysticks.  First we send 0xff to
   * the port to trigger the one shots, then we pause for a while
   * and finally we check to see if anything happened
   */
  outportb(JS_DATA, 0xff);
  __usleep(JS_TIMEOUT);
  j = inportb(JS_DATA) & JS_CH_MASK;

  if (j == JS_CH_MASK) {
    syslog(LOG_ERR, "%s no device connections found", js_sysmsg);
    return;
  }
  if (!(j & JS_CH_A)) {
    strcat(init_msg, "A");
    js_mask |= JS_CH_A;
    js_channels++;
  }	
  if (!(j & JS_CH_B)) {
    if (js_mask) {
      strcat(init_msg, ", ");
    }
    strcat(init_msg, "B");
    js_mask |= JS_CH_B;
    js_channels++;
  }	
  if (!(j & JS_CH_C)) {
    if (js_mask) {
      strcat(init_msg, ", ");
    }
    strcat(init_msg, "C");
    js_mask |= JS_CH_C;
    js_channels++;
  }	
  if (!(j & JS_CH_D)) {
    if (js_mask) {
      strcat(init_msg, ", ");
    }
    strcat(init_msg, "D");
    js_mask |= JS_CH_D;
    js_channels++;
  }
  syslog(LOG_INFO, "%s connections found on channel%s %s\n", 
  	 js_sysmsg, (js_channels == 1) ? "" : "s", init_msg);
@


1.2
log
@Fix warnings
@
text
@d5 1
a5 1
 * Last Update: 21st March 1994
d28 1
d224 1
a224 1
    syslog(LOG_ERR, "joystick: no device connections found\n");
d256 2
a257 2
  syslog(LOG_INFO, "joystick: connections found on channel%s %s\n", 
  	 (js_channels == 1) ? "" : "s", init_msg);
@


1.1
log
@Initial revision
@
text
@d187 4
a190 4
  pio_short(buffer, &jsd_ch_a);
  pio_short(buffer, &jsd_ch_b);
  pio_short(buffer, &jsd_ch_c);
  pio_short(buffer, &jsd_ch_d);
@
