/*
 * main.c
 *	Main handling for ttymon
 */
#include "ttymon.h"
#define DEFSHELL "/vsta/bin/sh"

static char *shell;		/* User's shell */
static port_name portname;	/* Name of server port */
static port_t monport;		/*  ...server side of port */

/*
 * serve()
 *	Server loop handling
 */
static void
serve(void)
{
	struct msg *m;

	/*
	 * Get a system message
	 */
	m = malloc(sizeof(struct msg));
	if (m == 0) {
		perror("ttymon: msg memory");
		exit(1);
	}

	/*
	 * Receive next
}

main(void)
{
	/*
	 * Look up shell
	 */
	shell = getenv("SHELL");
	if (shell == 0) {
		shell = DEFSHELL;
	}

	/*
	 * Get a server port
	 */
	monport = msg_port(0, &portname);
	if (monport < 0) {
		perror("ttymon port");
		exit(1);
	}

	/*
	 * Launch initial shell
	 */
	if (launch()) {
		exit(1);
	}

	/*
	 * Serve
	 */
	for (;;) {
		serve();
	}
}
