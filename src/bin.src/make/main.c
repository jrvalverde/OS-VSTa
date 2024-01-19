/*
 *	make [-f makefile] [-ins] [target(s) ...]
 *
 *	(Better than EON mk but not quite as good as UNIX make)
 *
 *	-f makefile name
 *	-i ignore exit status
 *	-n Pretend to make
 *	-p Print all macros & targets
 *	-q Question up-to-dateness of target.  Return exit status 1 if not
 *	-r Don't not use inbuilt rules
 *	-s Make silently
 *	-t Touch files instead of making them
 *	-m Change memory requirements (EON only)
 */

#include <stdio.h>
#include "h.h"

char *			myname;
char *			makefile;	/*  The make file  */
FILE *			ifd;		/*  Input file desciptor  */
bool			domake = TRUE;	/*  Go through the motions option  */
bool			ignore = FALSE;	/*  Ignore exit status option  */
bool			silent = FALSE;	/*  Silent option  */
bool			print = FALSE;	/*  Print debuging information  */
bool			rules = TRUE;	/*  Use inbuilt rules  */
bool			dotouch = FALSE;/*  Touch files instead of making  */
bool			quest = FALSE;	/*  Question up-to-dateness of file  */


void
main(argc, argv)
int			argc;
char **			argv;
{
	register char *		p;		/*  For argument processing  */
	int			estat = 0;	/*  For question  */
	register struct name *	np;


	myname = (argc-- < 1) ? "make" : *argv++;

	while ((argc > 0) && (**argv == '-'))
	{
		argc--;		/*  One less to process  */
		p = *argv++;	/*  Now processing this one  */

		while (*++p != '\0')
		{
			switch(*p)
			{
			case 'f':	/*  Alternate file name  */
				if (*++p == '\0')
				{
					if (argc-- <= 0)
						usage();
					p = *argv++;
				}
				makefile = p;
				goto end_of_args;
			case 'n':	/*  Pretend mode  */
				domake = FALSE;
				break;
			case 'i':	/*  Ignore fault mode  */
				ignore = TRUE;
				break;
			case 's':	/*  Silent about commands  */
				silent = TRUE;
				break;
			case 'p':
				print = TRUE;
				break;
			case 'r':
				rules = FALSE;
				break;
			case 't':
				dotouch = TRUE;
				break;
			case 'q':
				quest = TRUE;
				break;
			default:	/*  Wrong option  */
				usage();
			}
		}
	end_of_args:;
	}

	/*  Can use stdin as makefile  */
	if (makefile && strcmp(makefile, "-") == 0) {
		ifd = stdin;
	} else {
		if (!makefile) {	/*  If no file, then use default */
			ifd = fopen(DEFN1, "r");
			if ((ifd == (FILE *)0)
				  && ((ifd = fopen(DEFN2, "r")) == (FILE *)0))
				fatal("Can't open %s", DEFN2);
		} else {
			if ((ifd = fopen(makefile, "r")) == (FILE *)0)
				fatal("Can't open %s", makefile);
		}

	}
	makerules();

	setmacro("$", "$");

	while (argc && (p = index(*argv, '=')))
	{
		char		c;

		c = *p;
		*p = '\0';
		setmacro(*argv, p+1);
		*p = c;

		argv++;
		argc--;
	}

	input(ifd);	/*  Input all the gunga  */
	fclose(ifd);	/*  Finished with makefile  */
	lineno = 0;	/*  Any calls to error now print no line number */

	if (print)
		prt();	/*  Print out structures  */

	np = newname(".SILENT");
	if (np->n_flag & N_TARG)
		silent = TRUE;

	np = newname(".IGNORE");
	if (np->n_flag & N_TARG)
		ignore = TRUE;

	precious();

	if (!firstname)
		fatal("No targets defined");

	circh();	/*  Check circles in target definitions  */

	if (!argc)
		estat = make(firstname, 0);
	else while (argc--)
	{
		if (!print && !silent && strcmp(*argv, "love") == 0)
			printf("Not war!\n");
		estat |= make(newname(*argv++), 0);
	}

	if (quest)
		exit(estat);
	else
		exit(0);
}


usage()
{
	fprintf(stderr, "Usage: %s [-f makefile] [-inpqrst] [macro=val ...] [target(s) ...]\n", myname);
	exit(1);
}


void
fatal(msg, a1, a2, a3, a4, a5, a6)
char	*msg;
{
	fprintf(stderr, "%s: ", myname);
	fprintf(stderr, msg, a1, a2, a3, a4, a5, a6);
	fputc('\n', stderr);
	exit(1);
}
