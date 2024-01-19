head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.08.29.18.08.25;	author vandys;	state Exp;
branches;
next	;


desc
@Utility library
@


1.1
log
@Initial revision
@
text
@#include <sys/types.h>
#include <sys/param.h>

/***************************************************************************
  byte order conversion, nops for mips processor in big endian mode
***************************************************************************/

#if  defined(vax) || defined(i386)

ulong
htonl(hl)
        ulong hl;
{
        char nl[4];

        nl[0] = ((char *) &hl)[3];
        nl[1] = ((char *) &hl)[2];
        nl[2] = ((char *) &hl)[1];
        nl[3] = ((char *) &hl)[0];

        return (*(ulong *) nl);
}

ushort
htons(hs)
        ushort hs;
{
        char ns[2];

        ns[0] = ((char *) &hs)[1];
        ns[1] = ((char *) &hs)[0];

        return (*(ushort *) ns);
}

ulong
ntohl(nl)
        ulong nl;
{
        char hl[4];

        hl[0] = ((char *) &nl)[3];
        hl[1] = ((char *) &nl)[2];
        hl[2] = ((char *) &nl)[1];
        hl[3] = ((char *) &nl)[0];

        return (*(ulong *) hl);
}

ushort
ntohs(ns)
        ushort ns;
{
        char hs[2];

        hs[0] = ((char *) &ns)[1];
        hs[1] = ((char *) &ns)[0];

        return (*(ushort *) hs);
}

#endif
@
