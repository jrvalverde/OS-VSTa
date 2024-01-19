head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.1
date	94.03.28.23.10.47;	author vandys;	state Exp;
branches;
next	;


desc
@String seperator handling
@


1.1
log
@Initial revision
@
text
@/* Copyright (C) 1992, 1993 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <string.h>

char *strsep(char **pp, const char *delim)
{
  char *p, *q;

  if (!(p = *pp))
    return 0;
  if (q = strpbrk (p, delim))
    {
      *pp = q + 1;
      *q = '\0';
    }
  else
    *pp = 0;
  return p;
}
@
