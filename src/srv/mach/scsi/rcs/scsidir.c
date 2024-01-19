head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.10.05.19.40.47;	author vandys;	state Exp;
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
 * scsidir.c - functions for formatting SCSI DIRECT CCB's and CDB's.
 */
#include <stdio.h>
#include <std.h>
#include "cam.h"

/*
 * cam_fmt_drw_cdb6()
 *	Format a SCSI_DIRECT read/write 6-byte CDB.
 */
void	cam_fmt_drw_cdb6(opcode, lun, lbaddr, length, control, cdb)
long	opcode, lun, length, control;
uint32	lbaddr;
struct	scsi_drw_cdb6 *cdb;
{
	cam_fmt_cdb6(opcode, lun, length, control, (struct scsi_cdb6 *)cdb);
	cdb->lbaddr2 = (lbaddr >> 16) & 0x1f;
	cdb->lbaddr1 = (lbaddr >> 8) & 0xff;
	cdb->lbaddr0 = lbaddr & 0xff;
}

/*
 * cam_fmt_drw_cdb10()
 *	Format a SCSI_DIRECT read/write 10-byte CDB.
 */
void	cam_fmt_drw_cdb10(opcode, lun, lbaddr, length, control, cdb)
long	opcode, lun, length, control;
uint32	lbaddr;
struct	scsi_drw_cdb10 *cdb;
{
	cam_fmt_cdb10(opcode, lun, lbaddr, length, control,
	              (struct scsi_cdb10 *)cdb);
}

/*
 * cam_fmt_prevent_ccb
 *	Format a PREVENT/ALLOW CDB.
 */
void	cam_fmt_prevent_ccb(devid, prevent, ccb)
CAM_DEV	devid;
int	prevent;
CCB	*ccb;
{
	struct	scsi_cdb6 *cdb;

	cam_init_scsiio_ccb(devid, 0, ccb);
	ccb->scsiio.cdb_len = 6;
	cdb = (struct scsi_cdb6 *)ccb->scsiio.cdb;
	cam_fmt_cdb6(SCSI_PREVENT, ccb->header.lun, 0, 0, cdb);
	cdb->length = (prevent ? 1 : 0);
}

@
