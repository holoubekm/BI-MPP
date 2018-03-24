#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define BUF_SIZE 128
#define MAKE_DATA 200
#define SWAP_OUTPUT 201

static int mpp_open(struct inode *inode, struct file *filp) { 
	printk ("mpp_open\n");
	
	return 0; 
}

static int mpp_release(struct inode *inode, struct file *f) {
	printk ("mpp_release\n");
	return 0; 
}

static int output = 0;
static int repeat_cnt = 10;


static ssize_t mpp_read(struct file *f, char *buf, size_t size, loff_t *offset) {
	const int len = 64;
	char kbuf_out[len];
	
	printk("mpp_read\n");

	if(output)
		strcpy(kbuf_out, "Hello from kernel!");
	else 
		strcpy(kbuf_out, "Unix power!");

	copy_to_user(buf, kbuf_out, min(size, (size_t)strlen(kbuf_out)));

	if(repeat_cnt-- <= 0)
		return 0;
	return min(size, (size_t)strlen(kbuf_out)); 
}

static const int buf_written = 0;
static char kbuf_in[BUF_SIZE];
static ssize_t mpp_write(struct file *f, const char *buf, size_t size, loff_t *offset) {
	printk ("mpp_write\n");
	copy_from_user(kbuf_in, buf, min(size, (size_t)BUF_SIZE));
	printk("Received: %ldb\n", min(size, (size_t)BUF_SIZE));
	printk("\033[1;31m%s\033[0m\n", kbuf_in);
	return size; 
}

static long mpp_ioctl (struct file * f, unsigned int request, unsigned long param) {
	printk ("mpp_ioctl\n");
	switch(request) {
		case MAKE_DATA:
			repeat_cnt = param;
			break;
		case SWAP_OUTPUT:
			output = !output;
			break;
	}
	printk("ioctl: %d", request);
	return 0;
}


struct file_operations mpp_fops = {
	.owner = THIS_MODULE,
	.llseek = NULL,
	.read = mpp_read,
	.write = mpp_write,
	// .aio_read = NULL,
	// .aio_write = NULL,
	// .readdir = NULL,
	.poll = NULL,
	.unlocked_ioctl = mpp_ioctl,
	.compat_ioctl = NULL,
	.mmap = NULL,
	.open = mpp_open,
	.flush = NULL,
	.release = mpp_release,
	.fsync = NULL,
	.aio_fsync = NULL,
	.fasync = NULL,
	.lock = NULL,
	.sendpage = NULL,
	.get_unmapped_area = NULL,
	.check_flags = NULL,
	.flock = NULL,
	.splice_write = NULL,
	.splice_read = NULL,
	.setlease = NULL,
	.fallocate = NULL
};

static int major = 200;
static int __init mpp_module_init(void)
{
	
	printk ("MPP module is loaded\n");
	register_chrdev(major, "mpp", &mpp_fops);
	printk("major: %d", major);
	return 0;
}

static void __exit mpp_module_exit(void)
{
	unregister_chrdev(major, "mpp");
	printk ("MPP module is unloaded\n");
	return;
}

module_init(mpp_module_init);
module_exit(mpp_module_exit);

MODULE_LICENSE("GPL");
