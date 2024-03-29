head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.10.05.20.25.48;	author vandys;	state Exp;
branches;
next	;


desc
@Initial RCS checkin of Mike Larson's CDFS
@


1.1
log
@Initial revision
@
text
@/*
 *	$Id: iso.h,v 1.6 1993/10/28 17:38:42 ws Exp $
 */

#define ISODCL(from, to) (to - from + 1)

struct iso_volume_descriptor {
	char type[ISODCL(1,1)]; /* 711 */
	char id[ISODCL(2,6)];
	char version[ISODCL(7,7)];
	char data[ISODCL(8,2048)];
};

/* volume descriptor types */
#define ISO_VD_PRIMARY 1
#define ISO_VD_END 255

#define ISO_STANDARD_ID "CD001"
#define ISO_ECMA_ID     "CDW01"

struct iso_primary_descriptor {
	char type			[ISODCL (  1,   1)]; /* 711 */
	char id				[ISODCL (  2,   6)];
	char version			[ISODCL (  7,   7)]; /* 711 */
	char unused1			[ISODCL (  8,   8)];
	char system_id			[ISODCL (  9,  40)]; /* achars */
	char volume_id			[ISODCL ( 41,  72)]; /* dchars */
	char unused2			[ISODCL ( 73,  80)];
	char volume_space_size		[ISODCL ( 81,  88)]; /* 733 */
	char unused3			[ISODCL ( 89, 120)];
	char volume_set_size		[ISODCL (121, 124)]; /* 723 */
	char volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
	char logical_block_size		[ISODCL (129, 132)]; /* 723 */
	char path_table_size		[ISODCL (133, 140)]; /* 733 */
	char type_l_path_table		[ISODCL (141, 144)]; /* 731 */
	char opt_type_l_path_table	[ISODCL (145, 148)]; /* 731 */
	char type_m_path_table		[ISODCL (149, 152)]; /* 732 */
	char opt_type_m_path_table	[ISODCL (153, 156)]; /* 732 */
	char root_directory_record	[ISODCL (157, 190)]; /* 9.1 */
	char volume_set_id		[ISODCL (191, 318)]; /* dchars */
	char publisher_id		[ISODCL (319, 446)]; /* achars */
	char preparer_id		[ISODCL (447, 574)]; /* achars */
	char application_id		[ISODCL (575, 702)]; /* achars */
	char copyright_file_id		[ISODCL (703, 739)]; /* 7.5 dchars */
	char abstract_file_id		[ISODCL (740, 776)]; /* 7.5 dchars */
	char bibliographic_file_id	[ISODCL (777, 813)]; /* 7.5 dchars */
	char creation_date		[ISODCL (814, 830)]; /* 8.4.26.1 */
	char modification_date		[ISODCL (831, 847)]; /* 8.4.26.1 */
	char expiration_date		[ISODCL (848, 864)]; /* 8.4.26.1 */
	char effective_date		[ISODCL (865, 881)]; /* 8.4.26.1 */
	char file_structure_version	[ISODCL (882, 882)]; /* 711 */
	char unused4			[ISODCL (883, 883)];
	char application_data		[ISODCL (884, 1395)];
	char unused5			[ISODCL (1396, 2048)];
};

struct iso_directory_record {
	char length			[ISODCL (1, 1)]; /* 711 */
	char ext_attr_length		[ISODCL (2, 2)]; /* 711 */
	unsigned char extent		[ISODCL (3, 10)]; /* 733 */
	unsigned char size		[ISODCL (11, 18)]; /* 733 */
	char date			[ISODCL (19, 25)]; /* 7 by 711 */
	char flags			[ISODCL (26, 26)];
	char file_unit_size		[ISODCL (27, 27)]; /* 711 */
	char interleave			[ISODCL (28, 28)]; /* 711 */
	char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	char name_len			[ISODCL (33, 33)]; /* 711 */
	char name			[0];
};
/* can't take sizeof(iso_directory_record), because of possible alignment
   of the last entry (34 instead of 33) */
#define ISO_DIRECTORY_RECORD_SIZE	33

struct iso_extended_attributes {
	unsigned char owner		[ISODCL (1, 4)]; /* 723 */
	unsigned char group		[ISODCL (5, 8)]; /* 723 */
	unsigned char perm		[ISODCL (9, 10)]; /* 9.5.3 */
	char ctime			[ISODCL (11, 27)]; /* 8.4.26.1 */
	char mtime			[ISODCL (28, 44)]; /* 8.4.26.1 */
	char xtime			[ISODCL (45, 61)]; /* 8.4.26.1 */
	char ftime			[ISODCL (62, 78)]; /* 8.4.26.1 */
	char recfmt			[ISODCL (79, 79)]; /* 711 */
	char recattr			[ISODCL (80, 80)]; /* 711 */
	unsigned char reclen		[ISODCL (81, 84)]; /* 723 */
	char system_id			[ISODCL (85, 116)]; /* achars */
	char system_use			[ISODCL (117, 180)];
	char version			[ISODCL (181, 181)]; /* 711 */
	char len_esc			[ISODCL (182, 182)]; /* 711 */
	char reserved			[ISODCL (183, 246)];
	unsigned char len_au		[ISODCL (247, 250)]; /* 723 */
};

/* CD-ROM Format type */
enum ISO_FTYPE  { ISO_FTYPE_DEFAULT, ISO_FTYPE_9660, ISO_FTYPE_RRIP, ISO_FTYPE_ECMA };

#ifndef	ISOFSMNT_ROOT
#define	ISOFSMNT_ROOT	0
#endif

struct iso_mnt {
	int im_flags;
	
	int logical_block_size;
	int volume_space_size;
	struct vnode *im_devvp;
	char im_fsmnt[50];
	
	struct mount *im_mountp;
	dev_t im_dev;
	
	int im_bshift;
	int im_bmask;
	
	char root[ISODCL (157, 190)];
	int root_extent;
	int root_size;
	enum ISO_FTYPE  iso_ftype;
	
	int rr_skip;
	int rr_skip0;
};

#define VFSTOISOFS(mp)	((struct iso_mnt *)((mp)->mnt_data))

#define iso_blkoff(imp, loc) ((loc) & (imp)->im_bmask)
#define iso_lblkno(imp, loc) ((loc) >> (imp)->im_bshift)
#define iso_blksize(imp, ip, lbn) ((imp)->logical_block_size)
#define iso_lblktosize(imp, blk) ((blk) << (imp)->im_bshift)


int isofs_mount __P((struct mount *mp, char *path, caddr_t data,
	struct nameidata *ndp, struct proc *p));
int isofs_start __P((struct mount *mp, int flags, struct proc *p));
int isofs_unmount __P((struct mount *mp, int mntflags, struct proc *p));
int isofs_root __P((struct mount *mp, struct vnode **vpp));
int isofs_statfs __P((struct mount *mp, struct statfs *sbp, struct proc *p));
int isofs_sync __P((struct mount *mp, int waitfor));
int isofs_fhtovp __P((struct mount *mp, struct fid *fhp, struct vnode **vpp));
int isofs_vptofh __P((struct vnode *vp, struct fid *fhp));
int isofs_init __P(());

struct iso_node;
int iso_bmap __P((struct iso_node *ip, int lblkno, daddr_t *result)); 
int iso_blkatoff __P((struct iso_node *ip, off_t offset, struct buf **bpp)); 
int iso_iget __P((struct iso_node *xp, ino_t ino, int relocated,
		  struct iso_node **ipp, struct iso_directory_record *isodir));
int iso_iput __P((struct iso_node *ip)); 
int iso_ilock __P((struct iso_node *ip)); 
int iso_iunlock __P((struct iso_node *ip)); 
int isofs_mountroot __P((void)); 

extern inline int
isonum_711(p)
	unsigned char *p;
{
	return *p;
}

extern inline int
isonum_712(p)
	char *p;
{
	return *p;
}

extern inline int
isonum_721(p)
	unsigned char *p;
{
	return *p|((char)p[1] << 8);
}

extern inline int
isonum_722(p)
	unsigned char *p;
{
	return ((char)*p << 8)|p[1];
}

extern inline int
isonum_723(p)
	unsigned char *p;
{
	return isonum_721(p);
}

extern inline int
isonum_731(p)
	unsigned char *p;
{
	return *p|(p[1] << 8)|(p[2] << 16)|(p[3] << 24);
}

extern inline int
isonum_732(p)
	unsigned char *p;
{
	return (*p << 24)|(p[1] << 16)|(p[2] << 8)|p[3];
}

extern inline int
isonum_733(p)
	unsigned char *p;
{
	return isonum_731(p);
}

int isofncmp __P((unsigned char *fn, int fnlen,
		  unsigned char *isofn, int isolen));
void isofntrans __P((unsigned char *infn, int infnlen,
		     unsigned char *outfn, unsigned short *outfnlen,
		     int original, int assoc));

/*
 * Associated files have a leading '='.
 */
#define	ASSOCCHAR	'='
@
