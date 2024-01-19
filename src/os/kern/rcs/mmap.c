head	1.6;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.6
date	94.12.19.05.51.21;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.10.13.17.53.47;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.10.05.19.09.25;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.09.30.22.54.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.09.23.20.40.57;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.51.22;	author vandys;	state Exp;
branches;
next	;


desc
@mmap(2) interface for anonymous and physical memory mapping
@


1.6
log
@Clean up some warnings
@
text
@/*
 * mmap.c
 *	Primary user interface to the VM system
 */
#include <sys/mman.h>
#include <sys/vas.h>
#include <sys/pview.h>
#include <sys/pset.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/fs.h>
#include <sys/port.h>
#include <hash.h>

/* Flag that we're not allowing a map hash to be created */
#define NO_MAP_HASH ((struct hash *)1)

/*
 * mmap()
 *	Map something into the address space
 *
 * Many combinations of options are not allowed.
 */
void *
mmap(void *addr, ulong len, int prot, int flags,
	port_t port, ulong offset)
{
	struct proc *p = curthread->t_proc;
	struct vas *vas = p->p_vas;
	struct pview *pv;
	struct pset *ps;
	void *vaddr;

	/*
	 * Anonymous memory
	 */
	if (flags & MAP_ANON) {
		/*
		 * Keep it simple, eh?  Read-only ZFOD???
		 */
		if ((flags & (MAP_FILE|MAP_FIXED|MAP_PHYS)) ||
				!(prot & PROT_WRITE)) {
			err(EINVAL);
			return(0);
		}

		/*
		 * Get view/set
		 */
		ps = alloc_pset_zfod(btorp(len));
		pv = alloc_pview(ps);

		/*
		 * Flag view as from mmap().  We use this to keep
		 * munmap() honest; it only works on views which
		 * were created by the user.
		 */
		pv->p_prot |= PROT_MMAP;

		/*
		 * If shared, turn on bit
		 */
		if (flags & MAP_SHARED) {
			ps->p_flags |= PF_SHARED;
		}

		/*
		 * Try to attach
		 */
		pv->p_vaddr = addr;
		vaddr = attach_pview(vas, pv);
		if (vaddr == 0) {
			free_pview(pv);
		}
		return(vaddr);
	}

	/*
	 * Physical mapping
	 */
	if (flags & MAP_PHYS) {
		/*
		 * Sanity
		 */
		if (flags & (MAP_FILE|MAP_FIXED)) {
			err(EINVAL);
			return(0);
		}

		/*
		 * Protection
		 */
		if (!issys()) {
			return(0);
		}

		/*
		 * Get a physical pset, create a view, map it in.
		 */
		ps = physmem_pset(btop(addr), btorp(len));
		pv = alloc_pview(ps);
		pv->p_prot = PROT_MMAP;
		vaddr = attach_pview(vas, pv);
		if (vaddr == 0) {
			free_pview(pv);
		}
		return(vaddr);
	}

	/*
	 * Create read-only or copy-on-write view of a file
	 */
	if (flags & MAP_FILE) {
		struct portref *pr;

		/*
		 * If writing, make sure they're asking for COW; we
		 * don't support writing back to a server via mmap().
		 * It's nasty to do.
		 */
		if ((prot & PROT_WRITE) && !(flags & MAP_PRIVATE)) {
			err(EINVAL);
			return(0);
		}

		/*
		 * Get the portref.  add_map() creates new portrefs
		 * as necessary, so just use the open file.
		 */
		pr = find_portref(p, port);
		v_lock(&pr->p_lock, SPL0);

		/*
		 * Try to generate a view.  Return address or error.
		 */
		addr = add_map(vas, pr, addr,
			btorp(len + (offset - ptob(btop(offset)))),
			btop(offset), !(prot & PROT_WRITE));
		v_sema(&pr->p_sema);
		if (addr) {
			return(addr);
		}

		/* VVV fall down to common error case VVV */

	}

	err(EINVAL);
	return(0);
}

/*
 * munmap()
 *	Unmap a region given an address within
 */
int
munmap(void *vaddr, ulong len)
{
	struct vas *vas = curthread->t_proc->p_vas;
	struct pview *pv;
	struct pset *ps;

	/*
	 * Look for the view which matches
	 */
	pv = find_pview(vas, vaddr);
	if (!pv) {
		return(err(EINVAL));
	}
	ps = pv->p_set;

	/*
	 * Make sure it's an mmap()'ed view.  Turn off bit so once we
	 * release the lock and call detach_view(), we can't race with
	 * another thread coming into munmap().
	 */
	if ((pv->p_prot & PROT_MMAP) == 0) {
		v_lock(&ps->p_lock, SPL0);
		return(err(EBUSY));
	}
	pv->p_prot &= ~(PROT_MMAP);
	v_lock(&ps->p_lock, SPL0);

	/*
	 * Blow it away
	 */
	remove_pview(vas, vaddr);

	return(0);
}

/*
 * get_map_pset()
 *	Return pset view of named file
 *
 * Servers must support the FS_FID function in order to allow
 * mapping of files.
 */
static struct pset *
get_map_pset(struct portref *pr)
{
	struct pset *ps;
	long args[3];
	struct port *port;

	/*
	 * Hold mutex so we're the only one searching/updating the
	 * cache of mappings.  This also keeps the port from shutting
	 * on us.
	 */
	p_lock(&pr->p_lock, SPL0);
	port = pr->p_port;
	if (port) {
		if ((port->p_flags & P_CLOSING) ||
				(port->p_maps == NO_MAP_HASH)) {
			v_lock(&pr->p_lock, SPL0);
			port = 0;
		} else {
			p_sema_v_lock(&port->p_mapsema, PRIHI,
				&pr->p_lock);
		}
	}

	/*
	 * Try to get file ID.  This also gets us the file's
	 * size.
	 */
	if (!port) {
		return(0);
	}
	if (kernmsg_send(pr, FS_FID, args)) {
		v_sema(&port->p_mapsema);
		return(0);
	}

	/*
	 * If there's a hash already, search it for our ID.  Otherwise
	 * create a new hash and flag that we didn't find the pset.
	 */
	if (port->p_maps) {
		ps = hash_lookup(port->p_maps, args[0]);
	} else {
		/*
		 * New hash table.  Is 32 a good number?  Who knows?
		 */
		port->p_maps = hash_alloc(32);
		ps = 0;
	}

	/*
	 * If the file's changed size, invalidate the old executable
	 * cache so we can set up the new one.
	 */
	if (ps && (ps->p_len != args[1])) {
		(void)hash_delete(port->p_maps, args[0]);
		deref_pset(ps);
		ps = 0;
	}

	/*
	 * If no pset, create one and insert it.  The mapping in
	 * the hash counts as a reference.
	 */
	if (ps == 0) {
		struct portref *newpr;

		newpr = dup_port(pr);
		if (newpr == 0) {
			v_sema(&port->p_mapsema);
			return(0);
		}
		ps = alloc_pset_fod(newpr, args[1]);
		(void)hash_insert(port->p_maps, args[0], ps);
		ref_pset(ps);
	}

	/*
	 * Leave an extra ref on the pset so it won't go away
	 * while we add views to it.  Our caller will release
	 * this ref once he's done his own refs.
	 */
	ref_pset(ps);

	/*
	 * Release sema and return pset
	 */
	v_sema(&port->p_mapsema);
	return(ps);
}

/*
 * add_map()
 *	Add a mmap view of the given file
 *
 * Returns 0 on failure, attach address on success
 */
void *
add_map(struct vas *vas, struct portref *pr, void *vaddr, ulong len,
		ulong off, int readonly)
{
	struct pset *ps;
	struct pview *pv;
	int err;

	/*
	 * Get underlying pset; might be cached
	 */
	ps = get_map_pset(pr);
	if (ps == 0) {
		return(0);
	}

	/*
	 * Create a pview of the given size
	 */
	if (readonly) {
		/*
		 * Read-only is a simple view into the pset
		 */
		pv = alloc_pview(ps);
		pv->p_prot = PROT_RO;
		if (pv->p_len > off) {
			pv->p_off = off;
			pv->p_len -= off;
		}
	} else {
		struct pset *ps2;

		/*
		 * Read/write means the view is copy-on-write;
		 * insert a COW pset on top of the straight
		 * view.
		 */
		ps2 = alloc_pset_cow(ps, off, len);
		pv = alloc_pview(ps2);
		pv->p_prot = 0;
		pv->p_off = 0;
	}
	if (len < pv->p_len)
		pv->p_len = len;
	pv->p_vaddr = vaddr;

	/*
	 * Try to attach.  Throw away if it's rejected.  Most
	 * likely this is a bogus filemap, with an address
	 * which the HAT rejected.
	 */
	err = (attach_pview(vas, pv) == 0);
	if (err) {
		free_pview(pv);
	}

	/*
	 * Drop "placeholder" ref to pset (see get_map_pset())
	 */
	deref_pset(ps);
	return(err ? 0 : pv->p_vaddr);
}

/*
 * do_derefport()
 *	Release a cached access to a port
 */
static int
do_derefport(long keydummy, struct pset *ps, void *dummy)
{
	deref_pset(ps);
	return(0);
}

/*
 * mmap_cleanup()
 *	Clean up mapped file cache, if any
 */
void
mmap_cleanup(struct port *port)
{
	/*
	 * Take map mutex
	 */
	p_sema(&port->p_mapsema, PRIHI);

	/*
	 * No cache--nothing to do
	 */
	if (port->p_maps == 0) {
		v_sema(&port->p_mapsema);
		return;
	}

	/*
	 * Remove reference from pset for each entry
	 * in our cache.
	 */
	hash_foreach(port->p_maps, do_derefport, 0);

	/*
	 * Free the hash storage, flag port as shutting down
	 */
	hash_dealloc(port->p_maps);
	port->p_maps = NO_MAP_HASH;

	v_sema(&port->p_mapsema);
}

/*
 * unhash()
 *	Unhash any reference to the indicated hash #
 */
int
unhash(port_t arg_port, long arg_fid)
{
	struct port *port;
	struct proc *p = curthread->t_proc;

	/*
	 * Verify range
	 */
	if (arg_port < PROCOPENS) {
		return(err(EBADF));
	}
	arg_port -= PROCOPENS;
	if (arg_port >= PROCPORTS) {
		return(err(EBADF));
	}

	/*
	 * Interlock, get access to port, atomically switch
	 * to mapsema.
	 */
	(void)p_sema(&p->p_sema, PRILO);
	port = p->p_ports[arg_port];
	if (port == 0) {
		v_sema(&p->p_sema);
		return(err(EBADF));
	}
	p_lock(&port->p_lock, SPLHI);
	v_sema(&p->p_sema);
	p_sema_v_lock(&port->p_mapsema, PRIHI, &port->p_lock);

	/*
	 * If there's a map, delete entry from it
	 */
	if (port->p_maps && (port->p_maps != NO_MAP_HASH)) {
		struct pset *ps;

		ps = hash_lookup(port->p_maps, arg_fid);
		if (ps) {
			(void)hash_delete(port->p_maps, arg_fid);
			deref_pset(ps);
		}
	}

	/*
	 * Release mutex, return success
	 */
	v_sema(&port->p_mapsema);
	return(0);
}
@


1.5
log
@Fix priority of lock
@
text
@d157 1
a203 1
	struct portref *prmap;
d365 1
a365 1
static
d369 1
d411 1
@


1.4
log
@Fix memory leak; mmap() support already created a portref as
needed.
@
text
@d436 1
a436 1
	p_lock(&port->p_lock, SPL0);
@


1.3
log
@Allow zfod to be assigned addresses; needed to implement
shared libraries
@
text
@d115 1
a115 1
		struct portref *pr, *newpr;
d128 2
a129 3
		 * Get the portref, make our own copy of it.  Note
		 * mmap() doesn't "steal" the open file mapped, thus
		 * we need our own parallel connection to the server.
a132 2
		newpr = dup_port(pr);
		v_sema(&pr->p_sema);
d135 1
a135 3
		 * Try to generate a view.  If we fail, we need to
		 * shut down our private connection.  Return address
		 * or 0.
d137 1
a137 1
		addr = add_map(vas, newpr, addr,
d140 2
a141 5
		if (addr == 0) {
			shut_client(newpr);
			err(EINVAL);
			return(0);
		} else {
d144 3
@


1.2
log
@mmap() cleanup, add support for general mapped files
@
text
@a39 11
		 * We don't allow you to choose (yet).  When we do, it'll
		 * probably require an enhancement to attach_zfod_vaddr(),
		 * as we need to atmoically check and add the new view at
		 * the given vaddr.
		 */
		if (addr) {
			err(EINVAL);
			return(0);
		}

		/*
d71 1
@


1.1
log
@Initial revision
@
text
@d13 2
d16 3
d26 2
a27 1
mmap(void *addr, ulong len, int prot, int flags, int fd, ulong offset)
d29 2
a30 1
	struct vas *vas = curthread->t_proc->p_vas;
d120 44
d204 268
@
