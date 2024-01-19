head	1.2;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.2
date	94.10.12.04.02.37;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.03.22.19.35.39;	author vandys;	state Exp;
branches;
next	;


desc
@Dump sectors
@


1.2
log
@Fix indentation, share some hex dumping code which
also provides an ASCII dump on the side.
@
text
@/*
 * Filename:	dumpsect.c
 * Author:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Started:	6th March 1994
 * Last Update: 14th March 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description:	Utility to read a file sector.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * usage()
 *	When the user's tried something illegal we tell them what was valid
 */
static void
usage(char *util_name)
{
	fprintf(stderr, "Usage: %s <file_name> <sector_number>\n", util_name);
	exit(1);
}

/*
 * print_stat()
 *	Print the stat field of the specified file, returning 0 for success
 *
 * We do a few checks, and only display data that was actually read - in other
 * words we take some notice of EOF markers
 */
static int
print_sect(char *name, int sect)
{
	FILE *fp;
	char dat[512];
	int x;

	/*
	* Open the specified file
	*/
	fp = fopen(name, "rb");
	if (fp == NULL) {
		perror(name);
		return 1;
	}

	/*
	* Position at the appropriate sector
	*/
	fseek(fp, sect * 512, SEEK_SET);
	x = fread(dat, 1, 512, fp); 
	fclose(fp);

	/*
	* Display the sector contents
	*/
	if (x > 0) {
		dump_s(dat, x);
	}
	return(0);
}

/*
 * main()
 *	Sounds like a good place to start things :-)
 */
int
main(int argc, char **argv)
{
	int exit_code = 0;
	int x;

	/*
	* Do we have anything that looks vaguely reasonable
	*/
	if (argc < 3) {
		usage(argv[0]);
	}

	(void)sscanf(argv[2], "%d", &x);
	exit_code = print_sect(argv[1], x);

	return(exit_code);
}
@


1.1
log
@Initial revision
@
text
@a9 2


a13 1

d18 2
a19 1
static void usage(char *util_name)
d21 2
a22 2
  fprintf(stderr, "Usage: %s <file_name> <sector_number>\n", util_name);
  exit(1);
a24 1

d32 2
a33 1
static int print_sect(char *name, int sect)
d35 27
a61 42
  FILE *fd;
  int x = 0, y = 0, xh, rem, rd;
  char dat[512];
	
  /*
   * Open the specified file
   */
  fd = fopen(name, "rb");
  if (fd == NULL) {
    perror(name);
    return 1;
  }

  /*
   * Position at the appropriate sector
   */
  fseek(fd, sect * 512, SEEK_SET);
  rd = fread(dat, 1, 512, fd); 
  fclose(fd);
  xh = rd / 0x18;
  rem = rd % 0x18;

  /*
   * Display the sector contents
   */
  for(x = 0; x < xh; x++) {
    printf("%04x: ", (x * 0x18));
    for(y = 0; y < 0x18; y++) {
      printf("%02x ", (uchar)dat[(x * 0x18) + y]);
    }
    printf("\n");
  }

  if (rem) {
    printf("%04x: ", (x * 0x18));
    for(y = 0; y < rem; y++) {
      printf("%02x ", (uchar)dat[rd - rem + y]);
    }
    printf("\n");
  }

  return 0;
a63 1

d68 2
a69 1
int main(int argc, char **argv)
d71 14
a84 14
  int exit_code = 0;
  int x;
	
  /*
   * Do we have anything that looks vaguely reasonable
   */
  if (argc < 3) {
    usage(argv[0]);
  }

  sscanf(argv[2], "%d", &x);
  exit_code = print_sect(argv[1], x);
	
  return exit_code;
@
