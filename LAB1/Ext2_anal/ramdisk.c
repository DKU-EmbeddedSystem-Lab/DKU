#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/pagemap.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
#define KERNEL_6
#endif

static int major = 0;
static unsigned char memory_data[512 * 1024 * 1024];

/*
 * Copy n bytes from src to the brd starting at sector. Does not sleep.
 */
static void copy_to_brd(const void *src, sector_t sector, size_t n)
{
    memcpy(memory_data + (sector << 9), src, n);
}

/*
 * Copy n bytes to dst from the brd starting at sector. Does not sleep.
 */
static void copy_from_brd(void *dst, sector_t sector, size_t n)
{
    memcpy(dst, memory_data + (sector << 9), n);
}

/*
 * Process a single bvec of a bio.
 */
static int brd_do_bvec(struct page *page, unsigned int len, unsigned int off, unsigned int op, sector_t sector)
{
    void *mem;
    int err = 0;

    mem = kmap_atomic(page);
    if (!op_is_write(op)) {
        copy_from_brd(mem + off, sector, len);
        flush_dcache_page(page);
    } else {
        flush_dcache_page(page);
        copy_to_brd(mem + off, sector, len);
    }
    kunmap_atomic(mem);

    return err;
}

#ifdef KERNEL_6
void brd_submit_bio(struct bio *bio)
#else
static blk_qc_t brd_submit_bio(struct bio *bio)
#endif
{
    sector_t sector = bio->bi_iter.bi_sector;
    struct bio_vec bvec;
    struct bvec_iter iter;

    bio_for_each_segment(bvec, bio, iter) {
        unsigned int len = bvec.bv_len;
        int err;

        /* Don't support un-aligned buffer */
        WARN_ON_ONCE((bvec.bv_offset & (SECTOR_SIZE - 1)) ||
                     (len & (SECTOR_SIZE - 1)));

        err = brd_do_bvec(bvec.bv_page, len, bvec.bv_offset,
                          bio_op(bio), sector);
        if (err)
            goto io_error;
        sector += len >> SECTOR_SHIFT;
    }

    bio_endio(bio);
#ifdef KERNEL_6
    return;
#else
    return BLK_QC_T_NONE;
#endif
io_error:
    bio_io_error(bio);
#ifdef KERNEL_6
    return;
#else
    return BLK_QC_T_NONE;
#endif
}

static int brd_rw_page(struct block_device *bdev, sector_t sector, struct page *page, unsigned int op)
{
    int err;

    if (PageTransHuge(page))
        return -ENOTSUPP;
    err = brd_do_bvec(page, PAGE_SIZE, 0, op, sector);
    page_endio(page, op_is_write(op), err);
    return err;
}

static const struct block_device_operations brd_fops = {
        .owner =		THIS_MODULE,
        .submit_bio =		brd_submit_bio,
        .rw_page =		brd_rw_page,
};

unsigned long rd_size = CONFIG_BLK_DEV_RAM_SIZE;

static struct gendisk *disk;

static int brd_alloc(void)
{
    disk = blk_alloc_disk(NUMA_NO_NODE);
    if (!disk)
        goto out_free_dev;

    disk->major		= major;
    disk->first_minor	= 0;
    disk->minors	= 1;
    disk->fops		= &brd_fops;
#ifdef KERNEL_6
#else
    disk->flags		= GENHD_FL_EXT_DEVT;
#endif
    strcpy(disk->disk_name, "ramdisk");
    set_capacity(disk, 512*1024);

    blk_queue_physical_block_size(disk->queue, PAGE_SIZE);
    blk_queue_flag_set(QUEUE_FLAG_NONROT, disk->queue);
    blk_queue_flag_clear(QUEUE_FLAG_ADD_RANDOM, disk->queue);
#ifdef KERNEL_6
    if (add_disk(disk))
    	goto out_free_dev;
#else
    add_disk(disk);
#endif

    return 0;

    out_free_dev:
    return -ENOMEM;
}

static int __init brd_init(void)
{
    int err;

    major = register_blkdev(major, "ramdisk");
    if (major <= 0)
        return -EIO;

    err = brd_alloc();
    if (err)
        goto out_free;

    pr_info("ramdisk: module loaded\n");
    return 0;

    out_free:
    unregister_blkdev(major, "ramdisk");

    pr_info("ramdisk: module NOT loaded !!!\n");
    return err;
}

static void __exit brd_exit(void)
{
    unregister_blkdev(major, "ramdisk");

    del_gendisk(disk);
#ifdef KERNEL_6
    put_disk(disk);
#else
    blk_cleanup_disk(disk);
#endif

    pr_info("ramdisk: module unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(brd_init);
module_exit(brd_exit);
