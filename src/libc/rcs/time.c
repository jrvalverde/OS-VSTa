head	1.7;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.6
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.7
date	94.02.02.19.41.08;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.29.22.54.15;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.11.19.14.18;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.10.18.43.53;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.09.23.25.45;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.15.35.46;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.10.19.09.59;	author vandys;	state Exp;
branches;
next	;


desc
@Time handling stuff
@


1.7
log
@use int arg to avoid tedium of who has to include what
@
text
@/*
 * time.c
 *	Time-oriented services
 */
#include <sys/types.h>
#include <time.h>

#define HRSECS (60*60)
#define DAYSECS (24*HRSECS)
static char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static char *months[] =
	{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static int month_len[] =
	{31, 28, 31, 30,
	 31, 30, 31, 31,
	 30, 31, 30, 31};

/*
 * sleep()
 *	Suspend execution the given amount of time
 */
uint
sleep(uint secs)
{
	struct time t;

	time_get(&t);
	t.t_sec += secs;
	time_sleep(&t);
	return(0);
}

/*
 * usleep()
 *	Like sleep, but in microseconds
 */
__usleep(int usecs)
{
	struct time t;

	time_get(&t);
	t.t_usec += usecs;
	while (t.t_usec > 1000000) {
		t.t_sec += 1;
		t.t_usec -= 1000000;
	}
	time_sleep(&t);
	return(0);
}

/*
 * msleep()
 *	Like sleep, but milliseconds
 */
__msleep(int msecs)
{
	return(__usleep(msecs * 1000));
}

/*
 * time()
 *	Get time in seconds since 1990
 *
 * Yeah, I could've done it from 1970, but this gains me 20 years.
 * It also lets me skip some weirdness in the 70's, and even I think
 * in the 80's.  It should also piss off all the people who like
 * to write ~1500 lines of C just to tell the time.
 */
long
time(long *lp)
{
	struct time t;

	time_get(&t);
	if (lp) {
		*lp = t.t_sec;
	}
	return(t.t_sec);
}

/*
 * leap()
 *	Tell if year is the leap year
 */
static
leap(int year)
{
	return(((year - 1990) & 3) == 2);
}

/*
 * gmtime()
 *	Convert time to Greenwich
 */
struct tm *
gmtime(time_t *lp)
{
	static struct tm tm;
	time_t l = *lp;
	ulong len;

	/*
	 * Take off years until we reach the desired one
	 */
	tm.tm_year = 1990;
	for (;;) {
		if (leap(tm.tm_year)) {
			len = 366 * DAYSECS;
		} else {
			len = 365 * DAYSECS;
		}
		if (l < len) {
			break;
		}
		l -= len;
		tm.tm_year += 1;
	}

	/*
	 * Absolute count of days into year
	 */
	tm.tm_yday = l/DAYSECS;

	/*
	 * Take off months until we reach the desired month
	 */
	if (leap(tm.tm_year)) {
		month_len[1] = 29;	/* Feb. on leap year */
	}
	tm.tm_mon = 0;
	for (;;) {
		len = month_len[tm.tm_mon] * DAYSECS;
		if (l < len) {
			break;
		}
		l -= len;
		tm.tm_mon += 1;
	}

	/*
	 * Figure the day
	 */
	tm.tm_mday = l/DAYSECS;
	l = l % DAYSECS;

	/*
	 * Hour/minute/second
	 */
	tm.tm_hour = l / HRSECS;
	l = l % HRSECS;
	tm.tm_min = l / 60;
	l = l % 60;
	tm.tm_sec = l;

	/*
	 * Day of week is easier, at least until we get leap weeks
	 * where you get, say, two tuesdays in a row.  Hmmm... or
	 * two saturdays. :-)
	 *
	 * 1990 started on a monday.
	 */
	tm.tm_wday = *lp / DAYSECS;
	tm.tm_wday += 1;
	tm.tm_wday %= 7;

	/*
	 * Fluff
	 */
	tm.tm_isdst = 0;
	tm.tm_gmtoff = 0;
	tm.tm_zone = "GMT";

	/*
	 * Convert values to their defined basis
	 */
	tm.tm_mday += 1;
	tm.tm_year -= 1900;

	return(&tm);
}

/*
 * localtime()
 *	Just gmtime
 *
 * Yes, always in GMT.  Get a clue, it's all one big world.
 */
struct tm *
localtime(time_t *lp)
{
	return(gmtime(lp));
}

/*
 * ctime()
 *	Give printed string version of time
 */
char *
ctime(time_t *lp)
{
	register struct tm *tm;
	static char timebuf[32];

	/*
	 * Get basic time information
	 */
	tm = localtime(lp);

	/*
	 * Print it all into a buffer
	 */
	sprintf(timebuf, "%s %s %d, %d  %02d:%02d:%02d %s\n",
		days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, 1900 + tm->tm_year,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		tm->tm_zone);
	return(timebuf);
}

@


1.6
log
@Add correct type for fn
@
text
@d38 1
a38 1
__usleep(uint usecs)
d56 1
a56 1
__msleep(uint msecs)
@


1.5
log
@Convert all to gmtime() as the base function.  Add localtime.
Use a "struct tm" as our basic structure for interpreted time.
@
text
@d23 1
@


1.4
log
@Get function prototypes from <time.h>
@
text
@d92 2
a93 4
 * ctime()
 *	Give printed string version of time
 *
 * Always in GMT.  Get a clue, it's all one big world.
d95 2
a96 2
char *
ctime(long *lp)
d98 3
a100 3
	int day, month, year, dow, hr, min, sec;
	long l = *lp, len;
	static char timebuf[32];
d105 1
a105 1
	year = 1990;
d107 1
a107 1
		if (leap(year)) {
d116 1
a116 1
		year += 1;
d120 5
d127 1
a127 1
	if (leap(year)) {
d130 1
a130 1
	month = 0;
d132 1
a132 1
		len = month_len[month] * DAYSECS;
d137 1
a137 1
		month += 1;
d143 1
a143 1
	day = l/DAYSECS;
d149 1
a149 1
	hr = l / HRSECS;
d151 1
a151 1
	min = l / 60;
d153 1
a153 1
	sec = l;
d162 46
a207 3
	dow = *lp / DAYSECS;
	dow += 1;
	dow %= 7;
d212 5
a216 3
	sprintf(timebuf, "%s %s %d, %d  %02d:%02d:%02d\n",
		days[dow], months[month], day+1, year,
		hr, min, sec);
d219 1
@


1.3
log
@Add ctime() and all the noise having to do with mapping
linear time onto our calendar.
@
text
@d6 1
@


1.2
log
@Add micro and millisecond support
@
text
@d7 11
d57 112
@


1.1
log
@Initial revision
@
text
@d20 27
@
