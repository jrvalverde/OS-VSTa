head	1.2;
access;
symbols;
locks; strict;
comment	@ * @;


1.2
date	94.10.12.04.00.29;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.10.10.18.41.57;	author vandys;	state Exp;
branches;
next	;


desc
@Input line editing support
@


1.2
log
@Add getline() for canonical input processing
@
text
@#ifndef GETLINE_H
#define GETLINE_H

extern char *getline();		/* read a line of input */
extern void gl_setwidth();	/* specify width of screen */
extern void gl_histadd();	/* adds entries to hist */
extern void gl_strwidth();	/* to bind gl_strlen */

extern int (*gl_in_hook)();
extern int (*gl_out_hook)();
extern int (*gl_tab_hook)();

#endif /* GETLINE_H */
@


1.1
log
@Initial revision
@
text
@d4 4
a7 3
/* unix systems can #define POSIX to use termios, otherwise 
 * the bsd or sysv interface will be used 
 */
d9 3
a11 8
char           *getline();		/* read a line of input */
void            gl_setwidth();		/* specify width of screen */
void            gl_histadd();		/* adds entries to hist */
void		gl_strwidth();		/* to bind gl_strlen */

extern int 	(*gl_in_hook)();
extern int 	(*gl_out_hook)();
extern int	(*gl_tab_hook)();
@
