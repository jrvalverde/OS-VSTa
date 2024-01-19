#ifndef TTYMON_H
#define TTYMON_H
/*
 * ttymon.h
 *	Data structures for TTY handling monitor
 */
#include <sys/types.h>
#include <termios.h>

/*
 * Per-session state
 */
struct session {
	struct msg *s_msg;	/* Pending I/O request */
	struct termios		/* Configuration of terminal */
		s_term;		/*  ...for VINTR, etc. */
	pid_t s_pid;		/* PID of lead shell in session */
};

#endif /* TTYMON_H */
