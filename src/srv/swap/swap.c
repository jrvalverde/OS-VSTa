/*
 * swap.c
 *	Routines for tabulating swap space
 *
 * We use two tricks to simplify the organization of swap.  First,
 * we start swap blocks at 1, not 0, so that we may simply enter
 * swap block numbers in to the resource map (0 is the error value
 * for an rmap).
 *
 * Second, we always leave an unused block number between each chunk
 * of swap space added.  This means that the swap space for any given
 * virtual object will always reside on a single device.  This simplifies
 * the setup for I/O.
 */
#include <sys/swap.h>
#include <rmap.h>
#include <sys/assert.h>
#include <sys/msg.h>
#include <sys/seg.h>
#include <alloc.h>
#include <std.h>
#include <mnttab.h>
#include <fcntl.h>

#define NMAPELEM (200)		/* # slots in resource map */

struct swapmap *swapmap = 0;	/* The map of swap */
int nswap = 0;
struct swapmap *swapend = 0;
struct rmap *smap;		/* Map of swap currently free */
ulong total_swap = 0L,		/* Counts of swap blocks */
	free_swap = 0L;
ulong swap_pending = 0L;	/* Blocks alloc'ed before first swap */
				/*  partition configured */

/*
 * swapent()
 *	Given swap block, return pointer to swap entry
 */
struct swapmap *
swapent(uint block)
{
	struct swapmap *s, *send;

	if (!swapmap) {
		return(0);
	}
	for (s = swapmap; s < swapend; ++s) {
		if (block < (s->s_block + s->s_len)) {
			if (block < s->s_block) {
				return(0);
			}
			return(s);
		}
	}
	return(0);
}

/*
 * swapinit()
 */
void
swapinit(void)
{
	smap = malloc(NMAPELEM*sizeof(struct rmap));
	rmap_init(smap, NMAPELEM);
}

/*
 * swapadd()
 *	Add more swap space to map
 */
static void
swapadd(port_t port, ulong len, ulong off)
{
	struct swapmap *s;
	ulong low;

	/*
	 * Allocate more space for swap map
	 */
	if (!swapmap) {
		ASSERT(swap_pending < len, "swapadd: too much pending");
		low = 1L;
	} else {
		s = swapend-1;
		low = s->s_block + s->s_len + 1;
	}
	s = realloc(swapmap, sizeof(struct swapmap)*(nswap+1));
	ASSERT(s, "swapadd: out of core");
	nswap += 1;
	swapmap = s;

	/*
	 * Update end of swapmap pointer
	 */
	swapend = swapmap+nswap;

	/*
	 * Add our entry
	 */
	s = swapend-1;
	s->s_block = low;
	s->s_len = len;
	s->s_port = port;
	s->s_off = off;

	/*
	 * Add it to our resource map, too
	 */
	if (swap_pending) {
		rmap_free(smap, low + swap_pending, len - swap_pending);
		swap_pending = 0L;
	} else {
		rmap_free(smap, low, len);
	}
	free_swap += len;
	total_swap += len;
}

/*
 * swap_add()
 *	Add some swap space
 */
void
swap_add(struct msg *m, struct file *f, uint len)
{
	struct swapadd *sa;
	port_t p;
	int fd;
	ulong slen;

	/*
	 * Make sure they can modify swap
	 */
	if (!(f->f_perms & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Get the swapadd description
	 */
	if ((m->m_nseg != 1) || (len < sizeof(struct swapadd))) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	sa = (struct swapadd *)m->m_buf;

	/*
	 * Connect to the named server
	 */
	p = path_open(sa->s_path, ACC_READ | ACC_WRITE);
	if (p < 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * If there's no size specified, use the whole thing
	 */
	if (sa->s_len == 0) {
		extern char *rstat();

		slen = btop(atoi(rstat(p, "size")));
		slen -= sa->s_off;
	} else {
		slen = 0;
	}

	/*
	 * Add the swap space
	 */
#ifdef DEBUG
	printf("swap: add path '%s' len %d off %d\n",
		sa->s_path, slen, sa->s_off);
#endif
	swapadd(p, slen, sa->s_off);

	/*
	 * Return success
	 */
	m->m_nseg = m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * swap_alloc()
 *	Allocate some swap space
 */
void
swap_alloc(struct msg *m, struct file *f)
{
	ulong blocks, blkno;

	/*
	 * Need write access to consume swap
	 */
	if (!(f->f_perms & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Before first swap device is configured, run pending
	 * tally.
	 */
	blocks = m->m_arg;
	if (!swapmap) {
		m->m_arg = swap_pending + 1L;
		swap_pending += blocks;
	}

	/*
	 * Else pull blocks from resource map
	 */
	else if ((blkno = rmap_alloc(smap, blocks)) == 0) {
		/*
		 * Failure
		 */
		m->m_arg = 0;
	} else {
		/*
		 * Success--update count, return block #
		 */
		free_swap -= blocks;
		m->m_arg = blkno;
	}

	/*
	 * Send back block # allocated, or failure
	 */
	m->m_arg1 = m->m_buflen = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * swap_free()
 *	Return previously allocated swap
 */
void
swap_free(struct msg *m, struct file *f)
{
	if (!(f->f_perms & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
	}
	free_swap += m->m_arg1;
	rmap_free(smap, m->m_arg, m->m_arg1);
	m->m_arg = m->m_buflen = m->m_nseg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}
