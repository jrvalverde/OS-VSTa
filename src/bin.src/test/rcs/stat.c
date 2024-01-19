head	1.5;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.5
date	94.10.23.17.44.41;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.10.06.04.17.21;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.10.01.03.32.18;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.05.30.21.33.29;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.03.22.19.35.39;	author vandys;	state Exp;
branches;
next	;


desc
@Interface to stat()/wstat()
@


1.5
log
@Fix up indentation, fiddle error reporting
@
text
@/*
 * Filename:	stat.c
 * Author:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Started:	3rd February 1994
 * Last Update: 13th May 1994
 * Implemented:	GNU GCC version 1.42 (VSTa 1.3.1)
 *
 * Description:	Utility to read the status fields of a file.
 */
#include <fcntl.h>
#include <fdl.h>
#include <stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/fs.h>
#include <sys/msg.h>

extern port_t path_open(char *, int);

/*
 * usage()
 *	When the user's tried something illegal we tell them what was valid
 */
static void
usage(char *util_name)
{
	fprintf(stderr,
	  "Usage: %s [-r] [-s] [-v] [-w] [-p] <path_name> <fields_names...>\n",
	  util_name);
	exit(1);
}

/*
 * read_stat()
 *	Print a stat field from the specified file.
 *
 * If the specified field is NULL we pick up all of the details.  Returns 0
 * for success, non-zero otherwise.
 */
static int
read_stat(char *name, int use_port, char *field, int verbose)
{
	int fd, x = 0, y = 0;
	char *statstr, fmtstr[512];
	port_t pt;

	if (!use_port) {
		/*
		 * Open the named file, stat it and then close it!
		 */
		fd = open(name, O_RDONLY);
		if (fd < 0) {
			perror(name);
			return 1;
		}
		statstr = rstat(__fd_port(fd), field);
		close(fd);
	} else {
		pt = path_open(name, ACC_CHMOD);
		if (pt < 0) {
			perror(name);
			return 1;
		}
		statstr = rstat(pt, field);
		msg_disconnect(pt);
	}

	if (statstr == NULL) {
		perror(name);
		return 0;
	}

	/*
	 * Format the result in an appropriate way
	 */
	if (field) {
		if (verbose) {
		printf("\t%s=", field);
		}
		printf("%s\n", statstr);
	} else {
		/*
		 * Scan the stat string, expanding the first char in any line
		 * so that it is preceded by a tab if we're verbose!
		 */
		if (verbose) {
			fmtstr[y++] = '\t';
			while (statstr[x] != '\0') {
				fmtstr[y++] = statstr[x++];
				if (statstr[x - 1] == '\n') {
					fmtstr[y++] = '\t';
				}
			}
			fmtstr[y - 1] = '\0';
			printf("%s", fmtstr);
		} else {
			/*
			 * If we're not verbose, just dump the results out raw
			 */
			printf("%s", statstr);
		}
	}

	return(0);		/* Return back OK status */
}

/*
 * write_stat()
 *	Write a stat field to the specified file.
 */
static int
write_stat(char *name, int use_port, char *field, int verbose)
{
	int fd, rcode;
	char statstr[128];
	port_t pt;

	strcpy(statstr, field);
	strcat(statstr, "\n");
	if (verbose) {
		printf("\t%s", statstr);
	}

	if (!use_port) {
		/*
		 * Open the named file, stat it and then close it!
		 */
		fd = open(name, O_WRONLY);
		if (fd < 0) {
			perror(name);
			return 1;
		}
		rcode = wstat(__fd_port(fd), statstr);
		close(fd);
	} else {
		pt = path_open(name, ACC_CHMOD);
		if (pt < 0) {
			perror(name);
			return 1;
		}
		rcode = wstat(pt, statstr);
		msg_disconnect(pt);
	}
	if (rcode < 0) {
		perror(name);
	}

	return(rcode);
}

main(int argc, char **argv)
{
	int exit_code = 0, x, doing_wstat = 0, verbose = 0, using_port = 0;
	int c;
	port_t pt;

	/*
	 * Do we have anything that looks vaguely reasonable
	 */
	if (argc < 2) {
		usage(argv[0]);
	}

	/*
	 * Scan for command line options
	 */
	while ((c = getopt(argc, argv, "prsvw")) != EOF) {
		switch(c) {
		case 'p' :	/* Use port name instead of file name */
			using_port = 1;
			break;

		case 'r' :			/* Read stat info */
			doing_wstat = 0;
			break;

		case 's' :			/* Silent */
			verbose = 0;
			break;

		case 'v' :			/* Verbose */
			verbose = 1;
			break;

		case 'w' :			/* Write stat info */
			doing_wstat = 1;
			break;

		default :	/* Only errors should get us here? */
			usage(argv[0]);
		}
	}

	if (doing_wstat) {
		if (optind == argc) {
			usage(argv[0]);
		}

		/*
		 * Handle any necessary verbosity
		 */
		if (verbose) {
			printf("%s: sending FS_WSTAT message:\n",
				argv[optind]);
		}

		/*
		 * We're writing, so loop round writing stat codes
		 */
		for (x = optind + 1; x < argc; x++) {
			exit_code = write_stat(argv[optind], using_port,
				argv[x], verbose);
			if (exit_code) {
				return(exit_code);
			}
		}
	} else {
		/*
		 * Handle any necessary verbosity
		 */
		if (verbose) {
			printf("%s: sending FS_STAT message:\n",
				argv[optind]);
		}
		if (argc == optind) {
			usage(argv[0]);
		} else if (argc == optind + 1) {
			argc++;
			argv[optind + 1] = NULL;
		}

		/*
		 * We're reading, so loop round displaying
		 * the specified stat codes
		 */
		for (x = optind + 1; x < argc; x++) {
			exit_code = read_stat(argv[optind], using_port,
				argv[x], verbose);
			if (exit_code) {
				return(exit_code);
			}
		}		
	}
	return(0);
}
@


1.4
log
@Convert to <getopt.h>
@
text
@a9 2


a18 1

a20 1

d25 2
a26 1
static void usage(char *util_name)
d28 4
a31 4
  fprintf(stderr,
  	  "Usage: %s [-r] [-s] [-v] [-w] [-p] <path_name> <fields_names...>\n",
          util_name);
  exit(1);
a33 1

d41 2
a42 1
static int read_stat(char *name, int use_port, char *field, int verbose)
d44 60
a103 62
  int fd;
  int x = 0, y = 0;
  char *statstr;
  char fmtstr[512];
  port_t pt;
	
  if (!use_port) {
    /*
     * Open the named file, stat it and then close it!
     */
    fd = open(name, O_RDONLY);
    if (fd < 0) {
      perror(name);
      return 1;
    }
    statstr = rstat(__fd_port(fd), field);
    close(fd);
  } else {
    pt = path_open(name, ACC_CHMOD);
    if (pt < 0) {
      perror(name);
      return 1;
    }
    statstr = rstat(pt, field);
    msg_disconnect(pt);
  }

  if (statstr == NULL) {
    perror(name);
    return 0;
  }

  /*
   * Format the result in an appropriate way
   */
  if (field) {
    if (verbose) {
      printf("\t%s=", field);
    }
    printf("%s\n", statstr);
  } else {
    /*
     * Scan the stat string, expanding the first char in any line
     * so that it is preceded by a tab if we're verbose!
     */
    if (verbose) {
      fmtstr[y++] = '\t';
      while (statstr[x] != '\0') {
        fmtstr[y++] = statstr[x++];
        if (statstr[x - 1] == '\n') {
          fmtstr[y++] = '\t';
        }
      }
      fmtstr[y - 1] = '\0';
      printf("%s", fmtstr);
    } else {
      /*
       * If we're not verbose, just dump the results out raw
       */
      printf("%s", statstr);
    }
  }
d105 1
a105 1
  return 0;		/* Return back OK status */
a107 1

d112 2
a113 1
static int write_stat(char *name, int use_port, char *field, int verbose)
d115 33
a147 31
  int fd;
  int rcode;
  char statstr[128];
  port_t pt;
	
  strcpy(statstr, field);
  strcat(statstr, "\n");
  if (verbose) {
    printf("\t%s", statstr);
  }
	
  if (!use_port) {
    /*
     * Open the named file, stat it and then close it!
     */
    fd = open(name, O_WRONLY);
    if (fd < 0) {
      perror(name);
      return 1;
    }
    rcode = wstat(__fd_port(fd), statstr);
    close(fd);
  } else {
    pt = path_open(name, ACC_CHMOD);
    if (pt < 0) {
      perror(name);
      return 1;
    }
    rcode = wstat(pt, statstr);
    msg_disconnect(pt);
  }
d149 1
a149 1
  return rcode;
d152 1
a152 6

/*
 * main()
 *	Sounds like a good place to start things :-)
 */
int main(int argc, char **argv)
d154 92
a245 89
  int exit_code = 0;
  int x;
  char c;
  int doing_wstat = 0, verbose = 0, using_port = 0;
  port_t pt;

  /*
   * Do we have anything that looks vaguely reasonable
   */
  if (argc < 2) {
    usage(argv[0]);
  }

  /*
   * Scan for command line options
   */
  while ((c = getopt(argc, argv, "prsvw")) != EOF) {
    switch(c) {
    case 'p' :			/* Use port name instead of file name */
      using_port = 1;
      break;
      
    case 'r' :			/* Read stat info */
      doing_wstat = 0;
      break;

    case 's' :			/* Silent */
      verbose = 0;
      break;

    case 'v' :			/* Verbose */
      verbose = 1;
      break;
    
    case 'w' :			/* Write stat info */
      doing_wstat = 1;
      break;

    default :			/* Only error's should get us here? */
      usage(argv[0]);
    }
  }

  if (doing_wstat) {
    if (optind == argc) {
      usage(argv[0]);
    }

    /*
     * Handle any necessary verbosity
     */
    if (verbose) {
      printf("%s: sending FS_WSTAT message:\n", argv[optind]);
    }

    /*
     * We're writing, so loop round writing stat codes
     */
    for (x = optind + 1; x < argc; x++) {
      if ((exit_code = write_stat(argv[optind], using_port,
      				  argv[x], verbose))) {
        return exit_code;
      }
    }
  } else {
    /*
     * Handle any necessary verbosity
     */
    if (verbose) {
      printf("%s: sending FS_STAT message:\n", argv[optind]);
    }
    if (argc == optind) {
      usage(argv[0]);
    } else if (argc == optind + 1) {
      argc++;
      argv[optind + 1] = NULL;
    }

    /*
     * We're reading, so loop round displaying the specified stat codes
     */
    for (x = optind + 1; x < argc; x++) {
      if ((exit_code = read_stat(argv[optind], using_port,
      				 argv[x], verbose))) {
        return exit_code;
      }
    }		
  }
  return 0;
@


1.3
log
@Don't define optind for ourselves
@
text
@d17 1
@


1.2
log
@Add -p
@
text
@a164 1
  extern int optind;
@


1.1
log
@Initial revision
@
text
@d5 2
a6 2
 * Last Update: 14th March 1994
 * Implemented:	GNU GCC version 2.5.7
d15 1
a15 1
#include <std.h>
d17 2
d21 3
d31 1
a31 1
  	  "Usage: %s [-r] [-s] [-v] [-w] <file_name> <fields_names...>\n",
d44 1
a44 1
static int read_stat(char *name, char *field, int verbose)
d50 1
d52 19
a70 7
  /*
   * Open the named file, stat it and then close it!
   */
  fd = open(name, O_RDONLY);
  if (fd < 0) {
    perror(name);
    return 1;
d72 1
a72 2
  statstr = rstat(__fd_port(fd), field);
  close(fd);
d117 1
a117 1
static int write_stat(char *name, char *field, int verbose)
d122 1
d130 19
a148 7
  /*
   * Open the named file, stat it and then close it!
   */
  fd = open(name, O_WRONLY);
  if (fd < 0) {
    perror(name);
    return 1;
a149 2
  rcode = wstat(__fd_port(fd), statstr);
  close(fd);
d164 1
a164 1
  int doing_wstat = 0, verbose = 0;
d166 1
d178 1
a178 1
  while ((c = getopt(argc, argv, "rsvw")) != EOF) {
d180 4
d221 2
a222 1
      if ((exit_code = write_stat(argv[optind], argv[x], verbose))) {
d244 2
a245 1
      if ((exit_code = read_stat(argv[optind], argv[x], verbose))) {
@
