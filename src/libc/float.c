/*
 * float.c
 *	Floating-point support routines
 *
 * Technology from software:
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Hmmm... some of this might be DJ Delorie's.  Since I'm not going
 * to charge for this stuff I think I'm OK, but he should get credit
 * where it's due.
 */

#ifdef FLOAT_SUPPORT
/*
 * strtod()
 *	Convert string to double, advance string pointer
 */
double
strtod(char *s, char **sret)
{
	double r;	/* result */
	int e;		/* exponent */
	double d;	/* scale */
	int sign;	/* +- 1.0 */
	int esign, i, flags = 0;

	r = 0.0;
	sign = 1.0;
	e = 0;
	esign = 1;

	while ((*s == ' ') || (*s == '\t')) {
		s++;
	}

	if (*s == '+') {
		s++;
	} else if (*s == '-') {
		sign = -1;
		s++;
	}

	while ((*s >= '0') && (*s <= '9')) {
		flags |= 1;
		r *= 10.0;
		r += *s - '0';
		s++;
	}

	if (*s == '.') {
		d = 0.1;
		s++;
		while ((*s >= '0') && (*s <= '9')) {
			flags |= 2;
			r += d * (*s - '0');
			s++;
			d /= 10.0;
		}
	}

	if (flags == 0) {
		if (sret) {
			*sret = s;
		}
		return 0;
	}

	if ((*s == 'e') || (*s == 'E')) {
		s++;
		if (*s == '+') {
			s++;
		} else if (*s == '-') {
			*s++;
			esign = -1;
		}
		if ((*s < '0') || (*s > '9')) {
			if (sret) {
				*sret = s;
			}
			return r;
		}

		while ((*s >= '0') && (*s <= '9')) {
			e *= 10.0;
			e += *s - '0';
			s++;
		}
	}

	if (esign < 0) {
		for (i = 1; i <= e; i++) {
			r /= 10.0;
		}
	} else {
		for (i = 1; i <= e; i++) {
			r *= 10.0;
		}
	}

	if (sret) {
		*sret = s;
	}
	return(r * sign);
}

/*
 * atof()
 *	Like atoi(), but floating point result
 */
double
atof(ascii)
	char *ascii;
{
	return(strtod(ascii, (char **)0));
}

/*
 * ldexp()
 *	Convert floating value to power
 */
double
ldexp(double v, int e)
{
	if (e < 0) {
		for (;;) {
			v /= 2;
			if (++e == 0) {
				return(v);
			}
		}
	}
	while (e > 0) {
		v *= 2;
		e -= 1;
	}
	return(v);
}

#else /* dummy out--no FLOAT_SUPPORT */

/*
 * GCC 1.X seems to choke on an entirely empty file... sigh
 */
static int x;

#endif /* FLOAT_SUPPORT */
