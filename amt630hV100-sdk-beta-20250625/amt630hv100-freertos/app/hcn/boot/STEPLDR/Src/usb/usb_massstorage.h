#ifndef __USB_MASS_STORAGE_H
#define __USB_MASS_STORAGE_H


/* Part types */
#define PART_TYPE_UNKNOWN   0x00
#define PART_TYPE_MAC       0x01
#define PART_TYPE_DOS       0x02
#define PART_TYPE_ISO       0x03
#define PART_TYPE_AMIGA     0x04
#define PART_TYPE_EFI       0x05

/* device types */
#define DEV_TYPE_UNKNOWN    0xff    /* not connected */
#define DEV_TYPE_HARDDISK   0x00    /* harddisk */
#define DEV_TYPE_TAPE       0x01    /* Tape */
#define DEV_TYPE_CDROM      0x05    /* CD-ROM */
#define DEV_TYPE_OPDISK     0x07    /* optical disk */

#define PART_NAME_LEN 32
#define PART_TYPE_LEN 32
#define MAX_SEARCH_PARTITIONS 64

typedef unsigned long lbaint_t;

typedef struct {
	u8 b[16];
} efi_guid_t;


/* Interface types: */
enum if_type {
	IF_TYPE_UNKNOWN = 0,
	IF_TYPE_IDE,
	IF_TYPE_SCSI,
	IF_TYPE_ATAPI,
	IF_TYPE_USB,
	IF_TYPE_DOC,
	IF_TYPE_MMC,
	IF_TYPE_SD,
	IF_TYPE_SATA,
	IF_TYPE_HOST,
	IF_TYPE_NVME,
	IF_TYPE_EFI,

	IF_TYPE_COUNT,			/* Number of interface types */
};

#define BLK_VEN_SIZE		40
#define BLK_PRD_SIZE		20
#define BLK_REV_SIZE		8

/*
 * Identifies the partition table type (ie. MBR vs GPT GUID) signature
 */
enum sig_type {
	SIG_TYPE_NONE,
	SIG_TYPE_MBR,
	SIG_TYPE_GUID,

	SIG_TYPE_COUNT			/* Number of signature types */
};

/*
 * With driver model (CONFIG_BLK) this is uclass platform data, accessible
 * with dev_get_uclass_platdata(dev)
 */
struct blk_desc {
	/*
	 * TODO: With driver model we should be able to use the parent
	 * device's uclass instead.
	 */
	enum if_type	if_type;	/* type of the interface */
	int		devnum;		/* device number */
	unsigned char	part_type;	/* partition type */
	unsigned char	target;		/* target SCSI ID */
	unsigned char	lun;		/* target LUN */
	unsigned char	hwpart;		/* HW partition, e.g. for eMMC */
	unsigned char	type;		/* device type */
	unsigned char	removable;	/* removable device */
	lbaint_t	lba;		/* number of blocks */
	unsigned long	blksz;		/* block size */
	int		log2blksz;	/* for convenience: log2(blksz) */
	char		vendor[BLK_VEN_SIZE + 1]; /* device vendor string */
	char		product[BLK_PRD_SIZE + 1]; /* device product number */
	char		revision[BLK_REV_SIZE + 1]; /* firmware revision */
	enum sig_type	sig_type;	/* Partition table signature type */
	union {
		u32 mbr_sig;	/* MBR integer signature */
		efi_guid_t guid_sig;	/* GPT GUID Signature */
	};

	unsigned long	(*block_read)(struct blk_desc *block_dev,
				      lbaint_t start,
				      lbaint_t blkcnt,
				      void *buffer);
	unsigned long	(*block_write)(struct blk_desc *block_dev,
				       lbaint_t start,
				       lbaint_t blkcnt,
				       const void *buffer);
	unsigned long	(*block_erase)(struct blk_desc *block_dev,
				       lbaint_t start,
				       lbaint_t blkcnt);
	void		*priv;		/* driver private struct pointer */
	void		*disk_priv;
};

extern struct blk_desc usb_dev_desc[6];

#endif
