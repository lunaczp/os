/*
 * This file contains some defines for the AT-hd-controller.
 * Various sources. Check out some definitions (see comments with
 * a ques).
 */
#ifndef _HDREG_H
#define _HDREG_H

/* Hd controller regs. Ref: IBM AT Bios-listing */
#define HD_DATA		0x1f0	/* _CTL when writing */
#define HD_ERROR	0x1f1	/* see err-bits */
#define HD_NSECTOR	0x1f2	/* nr of sectors to read/write */
#define HD_SECTOR	0x1f3	/* starting sector */
#define HD_LCYL		0x1f4	/* starting cylinder */
#define HD_HCYL		0x1f5	/* high byte of starting cyl */
#define HD_CURRENT	0x1f6	/* 101dhhhh , d=drive, hhhh=head */
#define HD_STATUS	0x1f7	/* see status-bits */
#define HD_PRECOMP HD_ERROR	/* same io address, read=error, write=precomp */
#define HD_COMMAND HD_STATUS	/* same io address, read=status, write=cmd */

#define HD_CMD		0x3f6

/* Bits of HD_STATUS */
#define ERR_STAT	0x01
#define INDEX_STAT	0x02
#define ECC_STAT	0x04	/* Corrected error */
#define DRQ_STAT	0x08
#define SEEK_STAT	0x10
#define WRERR_STAT	0x20
#define READY_STAT	0x40
#define BUSY_STAT	0x80

/* Values for HD_COMMAND */
#define WIN_RESTORE		0x10
#define WIN_READ		0x20
#define WIN_WRITE		0x30
#define WIN_VERIFY		0x40
#define WIN_FORMAT		0x50
#define WIN_INIT		0x60
#define WIN_SEEK 		0x70
#define WIN_DIAGNOSE		0x90
#define WIN_SPECIFY		0x91

/* Bits for HD_ERROR */
#define MARK_ERR	0x01	/* Bad address mark ? */
#define TRK0_ERR	0x02	/* couldn't find track 0 */
#define ABRT_ERR	0x04	/* ? */
#define ID_ERR		0x10	/* ? */
#define ECC_ERR		0x40	/* ? */
#define	BBD_ERR		0x80	/* ? */

/**lux
 * MBR Partition Entry
 * see https://en.wikipedia.org/wiki/Master_boot_record
 * Offset(bytes)	Field Length	Description
 * +0	1 byte	Status or physical drive (bit 7 set is for active or bootable, old MBRs only accept 0x80. 0x00 means inactive, 0x01-0x7F stand for invalid.
 * +1	3 bytes	CHS address of first absolute sector in partition. The format is described by three bytes, see the next three rows.
 * 	+1	1 byte	head
 * 	+2	1 byte	sector in bits 5–0; bits 7–6 are high bits of cylinder
 * 	+3	1 byte	bits 7–0 of cylinder
 * +4	1 byte	Partition Type
 * +5	3 bytes	CHS address of last absolute sector in partition. The format is described by 3 bytes, see the next 3 rows.
 * 	+5
 * 	+6
 *  +7
 * +8	4 bytes	LBA of first absolute sector in the partition
 * +C	4 bytes	Number of sectors in partition
 */
struct partition {
	unsigned char boot_ind;		/* 0x80 - active (unused) */
	unsigned char head;		/* ? */
	unsigned char sector;		/* ? */
	unsigned char cyl;		/* ? */
	unsigned char sys_ind;		/* ? */
	unsigned char end_head;		/* ? */
	unsigned char end_sector;	/* ? */
	unsigned char end_cyl;		/* ? */
	unsigned int start_sect;	/* starting sector counting from 0 */
	unsigned int nr_sects;		/* nr of sectors in partition */
};

#endif
