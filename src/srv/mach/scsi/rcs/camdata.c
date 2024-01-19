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
 * camdata.c - CAM configuration information.
 */
#include "cam.h"

uint32	cam_debug_flags = 0;

long	sim154x_init(), sim154x_action();
CAM_SIM_ENTRY sim154x_entry = { sim154x_init, sim154x_action };

struct	cam_conftbl cam_conftbl[] = {
	{ &sim154x_entry },
};
long	cam_nconftbl_entries = sizeof(cam_conftbl) / sizeof(cam_conftbl[0]);

@
