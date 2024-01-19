/*
 *	Do the actual making for make
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stat.h>
#include <ctype.h>
#include <alloc.h>
#include "h.h"

/*
 *	Exec a shell that returns exit status correctly (/bin/esh).
 *	The standard EON shell returns the process number of the last
 *	async command, used by the debugger (ugg).
 */
static int
dosh(char *string)
{
	char *p, c, **argv;
	int argc, use_shell = 0;
	pid_t pid;
	const int maxargs = 400;

	/*
	 * Walk across the string and see if there appear to be
	 * any shell-special characters.  If so, we must use a shell,
	 * otherwise a fork/exec might suffice.
	 */
	for (p = string; c = *p; ++p) {
		if (isalnum(c) || isspace(c) || (c == '-') ||
				(c == '.') || (c == '/')) {
			continue;
		}
		use_shell = 1;
		break;
	}

	/*
	 * Nope, do it the same/simple/slow way
	 */
	if (use_shell) {
		return(system(string));
	}

	/*
	 * It's a command line which doesn't need a shell.  Burst
	 * out the arguments and run the command directly.
	 */
	p = alloca(strlen(string)+1);
	strcpy(p, string);
	argc = 0;
	argv = alloca(maxargs * sizeof(char *));
	if (!string || !argv) {
		fatal("dosh: no stack memory");
	}
	while (*p) {
		/*
		 * Find next word
		 */
		while (isspace(*p)) {
			++p;
		}

		/*
		 * Too many arguments, hand it off to a shell anyway
		 */
		if (argc >= maxargs) {
			return(system(string));
		}

		/*
		 * Next argument, add to array, null terminate
		 */
		argv[argc++] = p;
		while (*p && !isspace(*p)) {
			++p;
		}
		if (*p) {
			*p++ = '\0';
		}
	}
	argv[argc++] = 0;

	/*
	 * Launch new process, wait for child in parent
	 */
	pid = fork();
	if (pid == -1) {
		fatal(strerror());
	}
	if (pid != 0) {
		int st;

		waitpid(pid, &st, 0);
		return(WEXITSTATUS(st));
	}

	/*
	 * Use execv/execvp
	 */
	if (argv[0][0] == '/') {
		execv(argv[0], argv);
	} else {
		execvp(argv[0], argv);
	}
	perror(argv[0]);
	_exit(1);
}


/*
 *	Do commands to make a target
 */
void
docmds1(np, lp)
struct name *		np;
struct line *		lp;
{
	bool			ssilent;
	bool			signore;
	int			estat;
	register char *		q;
	register char *		p;
	register struct cmd *	cp;


	for (cp = lp->l_cmd; cp; cp = cp->c_next)
	{
		strcpy(str1, cp->c_cmd);
		expand(str1);
		q = str1;
		ssilent = silent;
		signore = ignore;
		while ((*q == '@') || (*q == '-'))
		{
			if (*q == '@')	   /*  Specific silent  */
				ssilent = TRUE;
			else		   /*  Specific ignore  */
				signore = TRUE;
			q++;		   /*  Not part of the command  */
		}

		if (!domake)
			ssilent = 0;

		if (!ssilent)
			fputs("    ", stdout);

		for (p=q; *p; p++)
		{
			if (*p == '\n' && p[1] != '\0')
			{
				*p = ' ';
				if (!ssilent)
					fputs("\\\n", stdout);
			}
			else if (!ssilent)
				putchar(*p);
		}
		if (!ssilent)
			putchar('\n');

		/*  Get the shell to execute it  */
		if (domake) {
			if ((estat = dosh(q)) != 0) {
				if (estat == -1)
					fatal("Couldn't execute %s", q);
				printf("%s: Error code %d", myname, estat);
				if (signore) {
					fputs(" (Ignored)\n", stdout);
				} else {
					putchar('\n');
					if (!(np->n_flag & N_PREC)) {
						if (unlink(np->n_name) == 0)
	printf("%s: '%s' removed.\n", myname, np->n_name);
					}
					exit(estat);
				}
			}
		}
	}
}

docmds(np)
	struct name *np;
{
	register struct line *	lp;

	for (lp = np->n_line; lp; lp = lp->l_next) {
		docmds1(np, lp);
	}
}

/*
 *	Get the modification time of a file.  If the first
 *	doesn't exist, it's modtime is set to 0.
 */
void
modtime(np)
	struct name *np;
{
	struct stat info;
	int fd;


	if (stat(np->n_name, &info) < 0) {
		np->n_time = 0L;
	} else {
		np->n_time = info.st_mtime;

		/* For dateless filesystems */
		if (np->n_time == 0L) {
			np->n_time = 1L;
		}
	}
}


/*
 *	Update the mod time of a file to now.
 */
void
touch(np)
	struct name *np;
{
	char c;
	int fd;

	if (!domake || !silent) {
		printf("    touch(%s)\n", np->n_name);
	}

	if (domake) {
		if ((fd = open(np->n_name, 2)) < 0)
			printf("%s: '%s' not touched - non-existant\n",
					myname, np->n_name);
		close(fd);
	}
}


/*
 *	Recursive routine to make a target.
 */
make(np, level)
	struct name *np;
	int level;
{
	register struct depend *dp, *qdp;
	register struct line *lp;
	time_t dtime = 1;
	bool didsomething = 0;

	if (np->n_flag & N_DONE)
		return 0;

	if (!np->n_time)
		modtime(np);		/*  Gets modtime of this file  */

	if (rules) {
		for (lp = np->n_line; lp; lp = lp->l_next)
			if (lp->l_cmd)
				break;
		if (!lp)
			dyndep(np);
	}

	if (!(np->n_flag & N_TARG) && np->n_time == 0L)
		fatal("Don't know how to make %s", np->n_name);

	for (qdp = (struct depend *)0, lp = np->n_line; lp; lp = lp->l_next)
	{
		for (dp = lp->l_dep; dp; dp = dp->d_next)
		{
			make(dp->d_name, level+1);
			if (np->n_time < dp->d_name->n_time)
				qdp = newdep(dp->d_name, qdp);
			dtime = max(dtime, dp->d_name->n_time);
		}
		if (!quest && (np->n_flag & N_DOUBLE) && (np->n_time < dtime))
		{
			make1(np, lp, qdp);	/* free()'s qdp */
			dtime = 1;
			qdp = (struct depend *)0;
			didsomething++;
		}
	}

	np->n_flag |= N_DONE;

	if (quest)
	{
		long		t;

		t = np->n_time;
		time(&np->n_time);
		return t < dtime;
	}
	else if (np->n_time < dtime && !(np->n_flag & N_DOUBLE))
	{
		make1(np, (struct line *)0, qdp);	/* free()'s qdp */
		time(&np->n_time);
	}
	else if (level == 0 && !didsomething)
		printf("%s: '%s' is up to date\n", myname, np->n_name);
	return 0;
}


make1(np, lp, qdp)
register struct depend *	qdp;
struct line *			lp;
struct name *			np;
{
	register struct depend *	dp;


	if (dotouch)
		touch(np);
	else
	{
		strcpy(str1, "");
		for (dp = qdp; dp; dp = qdp)
		{
			if (strlen(str1))
				strcat(str1, " ");
			strcat(str1, dp->d_name->n_name);
			qdp = dp->d_next;
			free(dp);
		}
		setmacro("?", str1);
		setmacro("@", np->n_name);

		/* R.D. 1987/02/01  add "$*" macro to stand for target minus suffix */
		/* I'm assuming that np->n_name is the full name of the target. */

#define	MAXNAMSIZ	32		/* allow 32-char names */
		{
			char tmpnam[MAXNAMSIZ];
			char *p;
			strcpy(tmpnam, np->n_name);
			p = tmpnam + strlen(tmpnam);			/* point p to end of tmpnam */
			while (*p != '.' && p != tmpnam)
				--p;
			/* now p points to dot, or tmpnam, or both */
			if (*p == '.')
				*p = '\0';								/* null out extension */

			/* now tmpnam holds target minus suffix */
			setmacro("*", tmpnam);
		}

		if (lp)		/* lp set if doing a :: rule */
			docmds1(np, lp);
		else
			docmds(np);
	}
}
