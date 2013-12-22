#include <linux/fs.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#define GLOBALMEM_SIZE 1024
struct class *globalmem_class = NULL;

static int major = 0;
static int minor = 0;

typedef struct globalmem_dev {
	struct cdev cdev;
	char *mem;
}globalmem_dev_type;
/*For config the memery size dynamicly by user
 * users should "echo size >"to change memory size
 * or use "cat "to get the size
*/
unsigned int globalmem_memory_size = GLOBALMEM_SIZE;
globalmem_dev_type *globalmem_devp;

static int globalmem_open(struct inode *inode,struct file *file)
{
	file->private_data = globalmem_devp;
	return 0;
}
static int globalmem_release(struct inode *inode,struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static ssize_t globalmem_read(struct file *file,char __user *buf,size_t count,loff_t *offset)
{
	int read_size = count;
	int ret;
	globalmem_dev_type *dev = (globalmem_dev_type *)file->private_data;
	if(*offset > globalmem_memory_size)
		return 0;
	if(read_size > globalmem_memory_size-*offset)
		read_size = globalmem_memory_size-*offset;
	if(read_size == 0)
		return -EFAULT;
	ret = copy_to_user(buf,dev->mem+*offset,read_size);
	if(ret < 0)
		return -EFAULT;
	*offset += read_size;
	return read_size;
}
static ssize_t globalmem_write(struct file *file,const char __user *buf,size_t count,loff_t *offset)
{
	int write_size = count;
	int ret;
	globalmem_dev_type *dev = (globalmem_dev_type *)file->private_data;
	if(*offset > globalmem_memory_size)
		return -ENOMEM;
	if(write_size > globalmem_memory_size-*offset)
		write_size = globalmem_memory_size-*offset;
	if(write_size == 0)
		return -EFAULT;
	ret = copy_from_user(dev->mem+*offset,buf,write_size);
	if(ret < 0)
		return -EFAULT;
	*offset += write_size;
	return write_size;
}

#define GLOBALMEM_MAGIC 0xf5
#define GLOBALMEM_CLR _IO(GLOBALMEM_MAGIC,0)

static int globalmem_ioctl(struct inode *inode,struct file *file,unsigned int cmd,unsigned long arg)
{
	printk(KERN_INFO"[kern] globalmem_clr code is 0x%x\n",GLOBALMEM_CLR);
	switch(cmd) {
	case 0x01:
		memset(file->private_data,0,globalmem_memory_size);
		printk(KERN_INFO"[kern]globalmem ioctl:MEM_CLEAR\n");
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
static loff_t globalmem_llseek(struct file *file,loff_t offset,int origin)
{
	switch(origin) {
		case SEEK_CUR:
			offset += file->f_pos;
			break;
		case SEEK_END:
			offset += globalmem_memory_size;
			break;
		default:
			return -EINVAL;
	}
	if(offset < 0 || offset > globalmem_memory_size)
		return -EINVAL;
//	printk(KERN_INFO"[kern]new_offset = %lld\n",new_offset);
	file->f_pos = offset;
	return offset;
}
const struct file_operations globalmem_fops = {
	.owner	= THIS_MODULE,
	.open 	= globalmem_open,
	.release	= globalmem_release,
	.read	= globalmem_read,
	.write	= globalmem_write,
	.ioctl		= globalmem_ioctl,
	.llseek	= globalmem_llseek,
};

static void globalmem_dev_setup(globalmem_dev_type *dev,int index)
{
	int err;
	dev_t devno = MKDEV(major,index);
	cdev_init(&dev->cdev,&globalmem_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev,devno,1);
	if(err)
		printk(KERN_INFO"[kern]cdev add failed!\n");
}
static int __init globalmem_init(void)
{
	int ret;
	dev_t devno;
	ret = alloc_chrdev_region(&devno,0,1,"globalmem");
	if(ret < 0)
		return ret;
	major = MAJOR(devno);
    minor = MINOR(devno);
    printk(KERN_INFO"[kern]globalmem regist major:%d,minor:%d\n",major,minor);

	globalmem_class = class_create(THIS_MODULE,"globalmem");
	if(IS_ERR(globalmem_class))
		return -1;
	device_create(globalmem_class,NULL,MKDEV(major,minor),"globalmem%d",minor);

	globalmem_devp = (globalmem_dev_type *)kzalloc(sizeof(globalmem_dev_type),GFP_KERNEL);
	if(globalmem_devp == NULL) {
		printk(KERN_INFO"[kern]globalmem malloc device memory failed!\n");
		goto malloc_failed;
	}
	globalmem_devp->mem = (char *)kzalloc(globalmem_memory_size*sizeof(char),GFP_KERNEL);
	if(globalmem_devp->mem == NULL) {
		printk(KERN_INFO"[kern]globalmem malloc memory failed!\n");
		goto malloc_failed;
	}
	globalmem_dev_setup(globalmem_devp,minor);
	printk(KERN_INFO"[kern]globalmem init ok!\n");
	return 0;
	
malloc_failed:
	unregister_chrdev_region(MKDEV(major,minor),1);
	return -ENOMEM;
}
static void __exit globalmem_exit(void)
{
	cdev_del(&(globalmem_devp->cdev));
	unregister_chrdev_region(MKDEV(major,minor),1);

	device_destroy(globalmem_class,MKDEV(major,minor));
	class_destroy(globalmem_class);

	kfree(globalmem_devp->mem);//first free the buff memory
	globalmem_devp->mem = NULL;
	kfree(globalmem_devp);//then free the device
	globalmem_devp = NULL;
	printk(KERN_INFO"[kern]globalmem exit ok!\n");
}

module_init(globalmem_init);
module_exit(globalmem_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Songchenglin");
MODULE_DESCRIPTION("A visual character device just like a RAM");
MODULE_ALIAS("Global memery");

