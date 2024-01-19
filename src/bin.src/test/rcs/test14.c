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
date	93.05.03.21.46.09;	author vandys;	state Exp;
branches;
next	;


desc
@Test of <pwd.h> routines
@


1.1
log
@Initial revision
@
text
@#include <stdio.h>
#include <pwd.h>

main()
{
	char buf[128];
	struct passwd *pwd;
	uid_t uid;

	for (;;) {
		printf("UID #: "); fflush(stdout);
		gets(buf);
		if (buf[0] == '\0') {
			exit(1);
		}
		uid = atoi(buf);
		pwd = getpwuid(uid);
		if (pwd == 0) {
			printf("UID %d not known\n", uid);
			continue;
		}
		printf(" -> %s\n", pwd->pw_name);
	}
}
@
