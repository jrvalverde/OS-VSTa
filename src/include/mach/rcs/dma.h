head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.01.29.16.04.28;	author vandys;	state Exp;
branches;
next	;


desc
@Definition of ISA/i386 DMA controller
@


1.1
log
@Initial revision
@
text
@#ifndef _DMA_H
#define _DMA_H
/*
 * Definitions for PC DMA controller
 */
#define DMA_LOW 0x4

#define DMA_ADDR 0x4
#define DMA_CNT 0x5
#define DMA_INIT 0xA
#define DMA_STAT0 0xB
#define DMA_STAT1 0xC
#define DMA_HIADDR 0x81

#define DMA_HIGH 0x81

/*
 * Operation masks for STAT registers
 */
#define DMA_READ 0x46
#define DMA_WRITE 0x4A

#endif /* _DMA_H */
@
