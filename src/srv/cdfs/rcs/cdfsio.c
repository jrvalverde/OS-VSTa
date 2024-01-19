head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.10.05.20.25.48;	author vandys;	state Exp;
branches;
next	;


desc
@Initial RCS checkin of Mike Larson's CDFS
@


1.1
log
@Initial revision
@
text
@/*
 * cdfsio.c - CDFS I/O functions. Interface between filesystem server
 * and block I/O library.
 */
#include "cdfs.h"

#ifndef	NULL
#define	NULL	0
#endif

/*
 * cdfs_getblk - get a pointer to the buffer structure associated with
 * the input starting block number. The number of blocks to get is
 * 'nblocks'(this currently must be 1). The 'data' output parameter
 * receives a pointer to the data buffer associated with the buffer
 * structure.
 */
void	*cdfs_getblk(cdfs, start, nblocks, data)
struct	cdfs *cdfs;
off_t	start;
int	nblocks;
void	**data;
{
	void	*bp;

	if(nblocks != 1)
		return(NULL);
	if((bp = bget(start)) == NULL)
		return(NULL);
	*data = bdata(bp);
	return(bp);
}

void	cdfs_relblk(struct cdfs *cdfs, void *cookie)
{
	bfree(cookie);
}

@
