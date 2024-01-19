head	1.4;
access;
symbols;
locks; strict;
comment	@ * @;


1.4
date	94.10.05.19.08.59;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.06.21.20.58.08;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.05.30.21.27.12;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.05.30.04.21.59;	author vandys;	state Exp;
branches;
next	;


desc
@@


1.4
log
@Updates from Dave Hudson, convert to standard indentation
@
text
@/*
 * Filename:	rw.c
 * Author:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Started:	12th March 1994
 * Last Update: 4th October 1994
 * Implemented:	GNU GCC 1.42 (VSTa v1.3.1)
 *
 * Description: Deals with read/write requests to the nvram
 */
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/fs.h>
#include <sys/msg.h>
#include <sys/seg.h>
#include <sys/syscall.h>
#include <mach/io.h>
#include "nvram.h"

/*
 * A few constants
 */
#define READING 1
#define WRITING 0
#define TIMEOUT_LOOP 1000000

/*
 * Variable definitions
 */
int nvram_bytes;		/* Number of bytes of NVRAM */
static uchar *nvram_buffer;	/* Buffer for NVRAM data transfers */

/*
 * do_readbyte()
 *	Read a byte from the NVRAM
 *
 * Returns non-zero on timeout, zero otherwise
 */
static int
do_readbyte(int addr, uchar *dbyte)
{
	/*
	 * If we're transferring in from the
	 * bottom 10 bytes we could be in the
	 * middle of a clock update, so we check
	 * and wait if necessary
	 */
	if (addr < 10) {
		int y = TIMEOUT_LOOP;

		do {
			/*
			 * Continue looping until there's no
			 * update in progress bit set in
			 * byte 0x0a of the NVRAM
			 */
			outportb(RTCSEL, 0x0a);
			y--;
		} while (y > 0 && (inportb(RTCDATA) & 0x80));

		if (y != TIMEOUT_LOOP - 1) {
			return 1;
		}

		outportb(RTCSEL, addr);
		*dbyte = inportb(RTCDATA);

		outportb(RTCSEL, 0x0a);
		if (inportb(RTCDATA) & 0x80) {
			return 1;
		}
	} else {
		outportb(RTCSEL, addr);
		*dbyte = inportb(RTCDATA);
	}
	return(0);
}

/*
 * do_writebyte()
 *	Write a byte to the NVRAM
 *
 * Returns non-zero on timeout, zero otherwise
 */
static int
do_writebyte(int addr, uchar dbyte)
{
	/*
	* If we're transferring in from the bottom 10 bytes we could be in the
	* middle of a clock update, so we check and wait if necessary
	*/
	if (addr < 10) {
		int y =  TIMEOUT_LOOP;

		do {
			/*
			 * Continue looping until there's
			 * no update in progress bit set in
			 * byte 0x0a of the NVRAM
			 */
			outportb(RTCSEL, 0x0a);
			y--;
		} while (y > 0 && (inportb(RTCDATA) & 0x80));

		if (y != TIMEOUT_LOOP - 1) {
			return 1;
		}

		outportb(RTCSEL, addr);
		outportb(RTCDATA, dbyte);

		outportb(RTCSEL, 0x0a);
		if (inportb(RTCDATA) & 0x80) {
			return 1;
		}
	} else {
		outportb(RTCSEL, addr);
		outportb(RTCDATA, dbyte);
	}
	return(0);
}

/*
 * do_rw_all()
 *	Handle reading/writing all of the nvram
 *
 * We return non-zero if we had a timeout failure (due to too many UIPs).
 * If there's no problem we return zero.
 */
static int
do_rw_all(int start, int count, uchar *tsfr_buffer, int reading)
{
	int x, retries = 0, ret;

retry_rw_all:
	for (x = 0; x < count; x++) {
		if (reading) {
			ret = do_readbyte(start + x, &tsfr_buffer[x]);
		} else {
			ret = do_writebyte(start + x, tsfr_buffer[x]);
		}
		if (ret) {
			/*
			 * Oops, we've hit an "update in
			 * progress" - start a retry
			 */
			retries++;
			if (retries >= 4) {
				/*
				 * Have we tried too many times?
				 * Flag fault if yes!
				 */
				syslog(LOG_ERR, "%s 'all' timed out",
					(reading ? "read" : "write"));
				return(1);
			} else {
				goto retry_rw_all;
			}
		}
	}
	return(0);
}

/*
 * do_rw_rtc()
 *	Handle reading/writing all of the RTC
 *
 * We return non-zero if we had a timeout failure (due to too many UIPs).
 * If there's no problem we return zero.
 */
static int
do_rw_rtc(int start, int count, uchar *tsfr_buffer, int reading)
{
	int x, retries = 0, ret;
	int seq[7] = {0x32, 0x09, 0x08, 0x07, 0x04, 0x02, 0x00};

retry_rw_rtc:
	for (x = 0; x < count; x++) {
		if (reading) {
			ret = do_readbyte(seq[start + x], &tsfr_buffer[x]);
		} else {
			ret = do_writebyte(seq[start + x], tsfr_buffer[x]);
		}
		if (ret) {
			/*
			 * Oops, we've hit an "update in
			 * progress" - start a retry
			 */
			retries++;
			if (retries >= 4) {
				/*
				 * Have we tried too many times?
				 * Flag fault if yes!
				 */
				syslog(LOG_ERR, "%s 'rtc' timed out",
					(reading ? "read" : "write"));
				return(1);
			} else {
				goto retry_rw_rtc;
			}
		}
	}
	return(0);
}

/*
 * Macro definitions
 */
#define CAP_TSFR(count, pos, size)	\
	if (count + pos > size) {	\
		count = size - pos;	\
	}

/*
 * nvram_readwrite()
 *	Read the contents of NVRAM
 */
void
nvram_readwrite(struct msg *m, struct file *f)
{
	int tsfr_count, ret = 0;
	uchar tbyte;

	/*
	 * Check that the transfer is sane
	 */
	if (m->m_arg == 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Check that the permissions are good
	 */
	if ((m->m_op == FS_WRITE) ?  (!(f->f_flags & ACC_WRITE)) :
			(!(f->f_flags & ACC_READ))) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * How many bytes are we going to transfer?
	 */
	tsfr_count = m->m_arg;

	/*
	 * Read/write the NVRAM via the transfer buffer.  We assume
	 * here that on * exit from whichever the appropriate
	 * routine is we return a status in * "ret".  If ret is
	 * non-zero we have a fail, otherwise we have a pass.
	 */
	if (m->m_op == FS_READ) {
		/*
		* Handle device reads
		*/
		switch(f->f_node) {
		case ROOT_INO :
			nvram_readdir(m, f);
			ret = 0;
			break;

		case ALL_INO :
			CAP_TSFR(tsfr_count, f->f_pos, nvram_bytes);
			ret = do_rw_all(f->f_pos, tsfr_count,
				nvram_buffer, READING);
			break;

		case RTC_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 7);
			ret = do_rw_rtc(f->f_pos, tsfr_count,
				nvram_buffer, READING);
			break;

		case FD0_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			ret = do_readbyte(0x10, nvram_buffer);
			nvram_buffer[0] >>= 4;
			break;

		case FD1_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			ret = do_readbyte(0x10, nvram_buffer);
			nvram_buffer[0] &= 0x0f;
			break;

		case WD0_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			ret = do_readbyte(0x12, nvram_buffer);
			nvram_buffer[0] >>= 4;
			if (nvram_buffer[0] == 0x0f) {
				ret = do_readbyte(0x19, nvram_buffer);
			}
			break;

		case WD1_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			ret = do_readbyte(0x12, nvram_buffer);
			nvram_buffer[0] &= 0x0f;
			if (nvram_buffer[0] == 0x0f) {
				ret = do_readbyte(0x1a, nvram_buffer);
			}
			break;

		default :
			msg_err(m->m_sender, EINVAL);
			return;
		}
	} else {
		/*
		* Handle device writes
		*/
		switch(f->f_node) {
		case ROOT_INO :
			msg_err(m->m_sender, EINVAL);
			return;

		case ALL_INO :
			CAP_TSFR(tsfr_count, f->f_pos, nvram_bytes);
			ret = do_rw_all(f->f_pos, tsfr_count,
				nvram_buffer, WRITING);
			break;

		case RTC_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 7);
			ret = do_rw_rtc(f->f_pos, tsfr_count,
				nvram_buffer, WRITING);
			break;

		case FD0_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			if (tsfr_count == 0) {
				ret = 0;
				break;
			}
			if (nvram_buffer[0] > 0x0f) {
				msg_err(m->m_sender, EINVAL);
				return;
			}
			ret = do_readbyte(0x10, &tbyte);
			tbyte = (tbyte & 0x0f) | (nvram_buffer[0] << 4);
			ret |= do_writebyte(0x10, tbyte);
			break;

		case FD1_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			if (tsfr_count == 0) {
				ret = 0;
				break;
			}
			if (nvram_buffer[0] > 0x0f) {
				msg_err(m->m_sender, EINVAL);
				return;
			}
			ret = do_readbyte(0x10, &tbyte);
			tbyte = (tbyte & 0xf0) | nvram_buffer[0];
			ret |= do_writebyte(0x10, tbyte);
			break;

		case WD0_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			if (tsfr_count == 0) {
				ret = 0;
				break;
			}
			if (nvram_buffer[0] > 0x0f) {
				ret = do_writebyte(0x19, nvram_buffer[0]);
				nvram_buffer[0] = 0xf0;
			}
			ret = do_readbyte(0x12, &tbyte);
			tbyte = (tbyte & 0x0f) | nvram_buffer[0];
			ret |= do_writebyte(0x12, tbyte);
			break;

		case WD1_INO :
			CAP_TSFR(tsfr_count, f->f_pos, 1);
			if (tsfr_count == 0) {
				ret = 0;
				break;
			}
			if (nvram_buffer[0] > 0x0f) {
				ret = do_writebyte(0x1a, nvram_buffer[0]);
				nvram_buffer[0] = 0x0f;
			}
			ret = do_readbyte(0x12, &tbyte);
			tbyte = (tbyte & 0xf0) | nvram_buffer[0];
			ret |= do_writebyte(0x12, tbyte);
			break;

		default :
			msg_err(m->m_sender, EINVAL);
			return;
		}
	}

	if (ret) {
		msg_err(m->m_sender, EBUSY);
		return;
	}

	f->f_pos += tsfr_count;

	/*
	* Post the results of the read/write back to the originator
	*/ 
	if ((m->m_op == FS_READ) && tsfr_count > 0) {
		m->m_buf = nvram_buffer;
		m->m_buflen = tsfr_count;
		m->m_nseg = 1;
	} else {
		m->m_nseg = 0;
	}
	m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * nvram_init()
 *	Initialise the "nvram" server.
 *
 * All we really need to do is decide how many bytes of NVRAM we're really
 * handling here - 64 is standard, but there could be 128 or 256
 */
void
nvram_init(void)
{
	uchar match[16];
	int x;
	uchar d;

	/*
	 * Look for any wrap arounds in the potential 256 byte data space - try
	 * to look for matches for bytes 16-31 (which are standard) at 80 and
	 * 144.  This will size at either 64, 128 or 256 bytes
	*/
	for (x = 0; x < 16; x++) {
		outportb(RTCSEL, x + 16);
		match[x] = inportb(RTCDATA);
	}

	/*
	 * Start searching at offset 144
	 */
	nvram_bytes = MAX_NVRAM_BYTES;
	x = 0;
	do {
		outportb(RTCSEL, x + 144);
		d = inportb(RTCDATA);
		x++;
	} while((x < 16) && (match[x - 1] == d)); 

	/*
	* If necessary, continue searching at offset 80
	*/
	if (x >= 16) {
		nvram_bytes = 128;
		x = 0;
		do {
			outportb(RTCSEL, x + 80);
			d = inportb(RTCDATA);
			x++;
		} while((x < 16) && (match[x - 1] == d)); 

		if (x >= 16) {
			nvram_bytes = MIN_NVRAM_BYTES;
		}
	}

	/*
	* Initialise the NVRAM transfer buffer
	*/
	nvram_buffer = malloc(MAX_NVRAM_BYTES);
	if (nvram_buffer == NULL) {
		syslog(LOG_ERR, "failed to allocate transfer buffer");
		exit(1);
	}

	syslog(LOG_INFO, "handling %d bytes", nvram_bytes);
}


/*
 * nvram_get_count_mode()
 *	Return the state of the count mode
 */
int
nvram_get_count_mode(void)
{
	uchar bcd_byte;

	do_readbyte(0x0b, &bcd_byte);
	return (bcd_byte & 0x04) ? 1 : 0;
}


/*
 * nvram_get_clock_mode()
 *	Return the state of the clock mode
 */
int
nvram_get_clock_mode(void)
{
	uchar clock_byte;

	do_readbyte(0x0b, &clock_byte);
	return (clock_byte & 0x02) ? 1 : 0;
}


/*
 * nvram_get_daysave_mode()
 *	Return the state of the daylight savings mode
 */
int nvram_get_daysave_mode(void)
{
	uchar daysave_byte;

	do_readbyte(0x0b, &daysave_byte);
	return (daysave_byte & 0x01) ? 1 : 0;
}
@


1.3
log
@Convert to openlog()
@
text
@d5 1
a5 1
 * Last Update: 10th May 1994
a9 2


a19 1

a26 1

d39 2
a40 1
static int do_readbyte(int addr, uchar *dbyte)
d42 35
a76 33
  /*
   * If we're transferring in from the bottom 10 bytes we could be in the
   * middle of a clock update, so we check and wait if necessary
   */
  if (addr < 10) {
    int y = TIMEOUT_LOOP;

    do { 
      /*
       * Continue looping until there's no update in progress bit set in
       * byte 0x0a of the NVRAM
       */
      outportb(RTCSEL, 0x0a);
      y--;
    } while (y > 0 && (inportb(RTCDATA) & 0x80));

    if (y != TIMEOUT_LOOP - 1) {
      return 1;
    }

    outportb(RTCSEL, addr);
    *dbyte = inportb(RTCDATA);

    outportb(RTCSEL, 0x0a);
    if (inportb(RTCDATA) & 0x80) {
      return 1;
    }
  } else {
    outportb(RTCSEL, addr);
    *dbyte = inportb(RTCDATA);
  }

  return 0;
a78 1

d85 2
a86 1
static int do_writebyte(int addr, uchar dbyte)
d88 33
a120 33
  /*
   * If we're transferring in from the bottom 10 bytes we could be in the
   * middle of a clock update, so we check and wait if necessary
   */
  if (addr < 10) {
    int y =  TIMEOUT_LOOP;

    do { 
      /*
       * Continue looping until there's no update in progress bit set in
       * byte 0x0a of the NVRAM
       */
      outportb(RTCSEL, 0x0a);
      y--;
    } while (y > 0 && (inportb(RTCDATA) & 0x80));

    if (y != TIMEOUT_LOOP - 1) {
      return 1;
    }

    outportb(RTCSEL, addr);
    outportb(RTCDATA, dbyte);

    outportb(RTCSEL, 0x0a);
    if (inportb(RTCDATA) & 0x80) {
      return 1;
    }
  } else {
    outportb(RTCSEL, addr);
    outportb(RTCDATA, dbyte);
  }
  
  return 0;
a122 1

d130 2
a131 1
static int do_rw_all(int start, int count, uchar *tsfr_buffer, int reading)
d133 1
a133 3
  int x;
  int retries = 0;
  int ret;
d136 26
a161 24
  for (x = 0; x < count; x++) {
    if (reading) {
      ret = do_readbyte(start + x, &tsfr_buffer[x]);
    } else {
      ret = do_writebyte(start + x, tsfr_buffer[x]);
    }
    if (ret) {
      /*
       * Oops, we've hit an "update in progress" - start a retry
       */
      retries++;
      if (retries >= 4) {
        /*
         * Have we tried too many times?  Flag fault if yes!
         */
        syslog(LOG_ERR, "%s 'all' timed out", (reading ? "read" : "write"));
        return 1;
      } else {
        goto retry_rw_all;
      }
    }
  }

  return 0;
a163 1

d171 2
a172 1
static int do_rw_rtc(int start, int count, uchar *tsfr_buffer, int reading)
d174 2
a175 4
  int x;
  int retries = 0;
  int ret;
  int seq[8] = {0x00, 0x02, 0x04, 0x06, 0x07, 0x08, 0x09, 0x32};
d178 26
a203 24
  for (x = 0; x < count; x++) {
    if (reading) {
      ret = do_readbyte(seq[start + x], &tsfr_buffer[x]);
    } else {
      ret = do_writebyte(seq[start + x], tsfr_buffer[x]);
    }
    if (ret) {
      /*
       * Oops, we've hit an "update in progress" - start a retry
       */
      retries++;
      if (retries >= 4) {
        /*
         * Have we tried too many times?  Flag fault if yes!
         */
        syslog(LOG_ERR, "%s 'rtc' timed out", (reading ? "read" : "write"));
        return 1;
      } else {
        goto retry_rw_rtc;
      }
    }
  }

  return 0;
a205 1

a213 1

d218 2
a219 1
void nvram_readwrite(struct msg *m, struct file *f)
d221 193
a413 190
  int tsfr_count;
  int ret = 0;
  uchar tbyte;

  /*
   * Check that the transfer is sane
   */
  if (m->m_arg == 0) {
    msg_err(m->m_sender, EINVAL);
    return;
  }

  /*
   * Check that the permissions are good
   */
  if ((m->m_op == FS_WRITE)
       ? (!(f->f_flags & ACC_WRITE)) : (!(f->f_flags & ACC_READ))) {
    msg_err(m->m_sender, EPERM);
    return;
  }

  /*
   * How many bytes are we going to transfer?
   */
  tsfr_count = m->m_arg;
  
  /*
   * Read/write the NVRAM via the transfer buffer.  We assume here that on
   * exit from whichever the appropriate routine is we return a status in
   * "ret".  If ret is non-zero we have a fail, otherwise we have a pass.
   */
  if (m->m_op == FS_READ) {
    /*
     * Handle device reads
     */
    switch(f->f_node) {
    case ROOT_INO :
      nvram_readdir(m, f);
      ret = 0;
      break;
      
    case ALL_INO :
      CAP_TSFR(tsfr_count, f->f_pos, nvram_bytes);
      ret = do_rw_all(f->f_pos, tsfr_count, nvram_buffer, READING);
      break;
      
    case RTC_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 8);
      ret = do_rw_rtc(f->f_pos, tsfr_count, nvram_buffer, READING);
      break;
      
    case FD0_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      ret = do_readbyte(0x10, nvram_buffer);
      nvram_buffer[0] >>= 4;
      break;
      
    case FD1_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      ret = do_readbyte(0x10, nvram_buffer);
      nvram_buffer[0] &= 0x0f;
      break;
      
    case WD0_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      ret = do_readbyte(0x12, nvram_buffer);
      nvram_buffer[0] >>= 4;
      if (nvram_buffer[0] == 0x0f) {
        ret = do_readbyte(0x19, nvram_buffer);
      }
      break;
      
    case WD1_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      ret = do_readbyte(0x12, nvram_buffer);
      nvram_buffer[0] &= 0x0f;
      if (nvram_buffer[0] == 0x0f) {
        ret = do_readbyte(0x1a, nvram_buffer);
      }
      break;

    default :
      msg_err(m->m_sender, EINVAL);
      return;
    }
  } else {
    /*
     * Handle device writes
     */
    switch(f->f_node) {
    case ROOT_INO :
      msg_err(m->m_sender, EINVAL);
      return;

    case ALL_INO :
      CAP_TSFR(tsfr_count, f->f_pos, nvram_bytes);
      ret = do_rw_all(f->f_pos, tsfr_count, nvram_buffer, WRITING);
      break;
      
    case RTC_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 8);
      ret = do_rw_rtc(f->f_pos, tsfr_count, nvram_buffer, WRITING);
      break;
      
    case FD0_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      if (tsfr_count == 0) {
        ret = 0;
        break;
      }
      if (nvram_buffer[0] > 0x0f) {
        msg_err(m->m_sender, EINVAL);
        return;
      }
      ret = do_readbyte(0x10, &tbyte);
      tbyte = (tbyte & 0x0f) | (nvram_buffer[0] << 4);
      ret |= do_writebyte(0x10, tbyte);
      break;
      
    case FD1_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      if (tsfr_count == 0) {
        ret = 0;
        break;
      }
      if (nvram_buffer[0] > 0x0f) {
        msg_err(m->m_sender, EINVAL);
        return;
      }
      ret = do_readbyte(0x10, &tbyte);
      tbyte = (tbyte & 0xf0) | nvram_buffer[0];
      ret |= do_writebyte(0x10, tbyte);
      break;
      
    case WD0_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      if (tsfr_count == 0) {
        ret = 0;
        break;
      }
      if (nvram_buffer[0] > 0x0f) {
        ret = do_writebyte(0x19, nvram_buffer[0]);
        nvram_buffer[0] = 0xf0;
      }
      ret = do_readbyte(0x12, &tbyte);
      tbyte = (tbyte & 0x0f) | nvram_buffer[0];
      ret |= do_writebyte(0x12, tbyte);
      break;
      
    case WD1_INO :
      CAP_TSFR(tsfr_count, f->f_pos, 1);
      if (tsfr_count == 0) {
        ret = 0;
        break;
      }
      if (nvram_buffer[0] > 0x0f) {
        ret = do_writebyte(0x1a, nvram_buffer[0]);
        nvram_buffer[0] = 0x0f;
      }
      ret = do_readbyte(0x12, &tbyte);
      tbyte = (tbyte & 0xf0) | nvram_buffer[0];
      ret |= do_writebyte(0x12, tbyte);
      break;
      
    default :
      msg_err(m->m_sender, EINVAL);
      return;
    }
  }

  if (ret) {
    msg_err(m->m_sender, EBUSY);
    return;
  }

  f->f_pos += tsfr_count;

  /*
   * Post the results of the read/write back to the originator
   */ 
 
  if ((m->m_op == FS_READ) && tsfr_count > 0) {
    m->m_buf = nvram_buffer;
    m->m_buflen = tsfr_count;
    m->m_nseg = 1;
  } else {
    m->m_nseg = 0;
  }
  m->m_arg = m->m_arg1 = 0;
  msg_reply(m->m_sender, m);
a415 1

d423 2
a424 1
void nvram_init(void)
d426 50
a475 50
  uchar match[16];
  int x;
  uchar d;

  /*
   * Look for any wrap arounds in the potential 256 byte data space - try
   * to look for matches for bytes 16-31 (which are standard) at 80 and
   * 144.  This will size at either 64, 128 or 256 bytes
   */
  for (x = 0; x < 16; x++) {
    outportb(RTCSEL, x + 16);
    match[x] = inportb(RTCDATA);
  }

  /*
   * Start searching at offset 144
   */
  nvram_bytes = MAX_NVRAM_BYTES;
  x = 0;
  do {
    outportb(RTCSEL, x + 144);
    d = inportb(RTCDATA);
    x++;
  } while((x < 16) && (match[x - 1] == d)); 

  /*
   * If necessary, continue searching at offset 80
   */
  if (x >= 16) {
    nvram_bytes = 128;
    x = 0;
    do {
      outportb(RTCSEL, x + 80);
      d = inportb(RTCDATA);
      x++;
    } while((x < 16) && (match[x - 1] == d)); 
    
    if (x >= 16) {
      nvram_bytes = MIN_NVRAM_BYTES;
    }
  }

  /*
   * Initialise the NVRAM transfer buffer
   */
  if ((nvram_buffer = (uchar *)malloc(sizeof(uchar) 
  				      * MAX_NVRAM_BYTES)) == NULL) {
    syslog(LOG_ERR, "failed to allocate transfer buffer");
    exit(1);
  }
d477 1
a477 1
  syslog(LOG_INFO, "handling %d bytes", nvram_bytes);
d485 2
a486 1
int nvram_get_count_mode(void)
d488 4
a491 4
  uchar bcd_byte;
  
  do_readbyte(0x0b, &bcd_byte);
  return (bcd_byte & 0x04) ? 1 : 0;
d499 2
a500 1
int nvram_get_clock_mode(void)
d502 4
a505 4
  uchar clock_byte;
  
  do_readbyte(0x0b, &clock_byte);
  return (clock_byte & 0x02) ? 1 : 0;
d515 4
a518 4
  uchar daysave_byte;
  
  do_readbyte(0x0b, &daysave_byte);
  return (daysave_byte & 0x01) ? 1 : 0;
@


1.2
log
@Syslog support, bug fix
@
text
@a36 3
extern char nvram_sysmsg[];


d154 1
a154 2
        syslog(LOG_ERR, "%s %s 'all' timed out",
               nvram_sysmsg, (reading ? "read" : "write"));
d196 1
a196 2
        syslog(LOG_ERR, "%s %s 'rtc' timed out",
               nvram_sysmsg, (reading ? "read" : "write"));
d472 1
a472 1
    syslog(LOG_ERR, "%s failed to allocate transfer buffer", nvram_sysmsg);
d476 1
a476 1
  syslog(LOG_INFO, "%s handling %d bytes", nvram_sysmsg, nvram_bytes);
@


1.1
log
@Initial revision
@
text
@d37 1
d39 1
d157 2
a158 2
        syslog(LOG_ERR, "nvram: %s 'all' timed out",
               (reading ? "read" : "write"));
d200 2
a201 2
        syslog(LOG_ERR, "nvram: %s 'rtc' timed out",
               (reading ? "read" : "write"));
d477 1
a477 1
    syslog(LOG_ERR, "nvram: failed to allocate transfer buffer");
d481 1
a481 1
  syslog(LOG_INFO, "nvram: server handling %d bytes", nvram_bytes);
@
