head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.10.05.19.40.45;	author vandys;	state Exp;
branches;
next	;


desc
@Initial RCS check-in of Mike Larson's SCSI subsystem
@


1.1
log
@Initial revision
@
text
@/*
 * camtimer.c - CAM timer related functions.
 */
#include <stdio.h>
#include <std.h>
#include "cam.h"

/*
 * Cam_proc_timer - timeout handler.
 */
void	cam_proc_timer(void)
{
}


@
