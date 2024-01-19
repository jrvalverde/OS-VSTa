head	1.2;
access;
symbols;
locks; strict;
comment	@ * @;


1.2
date	94.12.21.16.47.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.12.19.15.43.38;	author vandys;	state Exp;
branches;
next	;


desc
@Process status display
@


1.2
log
@Mount /proc within the command
@
text
@/*
 * ps.c
 *	Report process status
 */
#include "ps.h"
#include <dirent.h>

/*
 * sort()
 *	Tell qsort() to sort by PID value
 */
static int
sort(void *v1, void *v2)
{
	const pid_t p1 = *(const pid_t *)v1;
	const pid_t p2 = *(const pid_t *)v2;
	return p1 - p2;
}

int
main(int argc, char **argv)
{
	DIR *d;
	struct dirent *de;
	pid_t *pids;
	int space = 32;
	int nelem = 0;
	int i;
	
	/*
	 * Set up, get ready to read through list of all processes
	 */
	mount_procfs();
	d = opendir("/proc");
	if (d == 0)
		exit(1);

	pids = malloc(space * sizeof(pid_t));
	if (pids == 0) {
		perror("malloc");
		exit(1);
	}

	/*
	 * Read list
	 */
	while (de = readdir(d)) {
		if (!strcmp(de->d_name, "kernel")) {
			continue;
		}
		pids[nelem] = atoi(de->d_name);
		nelem++;
		if (nelem == space) {
			space += 32;
			pids = realloc(pids, space * sizeof(pid_t));
			if (pids == 0) {
				perror("realloc");
				exit(1);
			}
		}
	}
	closedir(d);

	/*
	 * Sort by PID
	 */
	qsort((void *)pids, nelem, sizeof(pid_t), sort);

	/*
	 * Now dump them
	 */
	for (i = 0; i < nelem; i++) {
		char path[32];
		char status[128];
		int fd, n;

		sprintf(path, "/proc/%d/status", pids[i]);
		fd = open(path, O_RDONLY);
		if (fd == -1) {
			continue;
		}
		n = read(fd, status, sizeof(status));
		status[n] = '\0';
		printf(status);
		close(fd);
	}
	return(0);
}
@


1.1
log
@Initial revision
@
text
@d1 5
a6 3
#include <stdlib.h>
#include <sys/fs.h>
#include <fcntl.h>
d8 4
d30 4
d35 1
a35 2
	if (d == 0) {
		perror("/proc");
a36 1
	}
d43 4
d64 3
d69 3
@
