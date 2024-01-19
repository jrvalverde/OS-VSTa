head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2;
locks; strict;
comment	@:: @;


1.3
date	94.04.06.21.58.50;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.11.16.02.51.27;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.10.25.22.22.30;	author vandys;	state Exp;
branches;
next	;


desc
@Run the microkernel
@


1.3
log
@Convert to install-oriented bootup
@
text
@boot vsta
@


1.2
log
@Source reorg
@
text
@d1 1
a1 1
boot ../os/make/vsta
@


1.1
log
@Initial revision
@
text
@d1 1
a1 1
boot ../make/vsta
@
