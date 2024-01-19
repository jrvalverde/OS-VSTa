head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@;; @;


1.1
date	93.06.08.04.20.18;	author vandys;	state Exp;
branches;
next	;


desc
@Various support stuff in assembly
@


1.1
log
@Initial revision
@
text
@;
; cputype.asm
;	Routine for determining CPU type
;
; I got this via djgpp, apparently it's originally in the i486
; manual.  I don't have one of those, so I have to trust djgpp
; for the lineage.
;

;
; Prologue
;
	DOSSEG
	.MODEL  large
	.386p

	.CODE
	
	PUBLIC	_cputype	; from Intel 80486 reference manual
_cputype PROC
	pushf
	pop	bx
	and	bx,0fffh
	push	bx
	popf
	pushf
	pop	ax
	and	ax,0f000h
	cmp	ax,0f000h
	jz	bad_cpu
	or	bx,0f000h
	push	bx
	popf
	pushf
	pop	ax
	and	ax,0f000h
	jz	bad_cpu

	smsw	ax
	test	ax,1
	jnz	bad_mode
	mov	ax,0
	ret

bad_mode:
	mov	ax,2
	ret

bad_cpu:
	mov	ax,1
	ret

_cputype ENDP

	END
@
