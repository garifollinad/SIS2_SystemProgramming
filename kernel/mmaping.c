#include <linux/init.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>

#include "mmaping.h"

MODULE_LICENSE("GPL");


//user space device interface (/dev)
//Memory areas are represented by a memory area object, which is stored in the vm_area_struct structure and defined in <linux/mm.h>
struct vma_data{
    atomic_t counter;/*number of free pages allocated, a free page has ->_count = -1.  This is so that we
     can use atomic_add_negative(-1, page->_count) to detect when the page becomes free*/
    struct page * page_address;
} this;

static struct file_operations/*Структура file_operations определена в файле linux/fs.h и содержит указатели на функции драйвера, 
которые отвечают за выполнение различных операций с устройством*/
dev_ops =
{
    .mmap = mmaping,
};

static struct miscdevice
dev_handle =
{
    .minor = MISC_DYNAMIC_MINOR,//number being registered, link btw /dev and misc driver
    .name = MODULE_NAME,//name for device
    .fops = &dev_ops,//pointer to file opertaions to act on a device
};

static int
paging_init_mod(void)
{

    struct vma_data *vma_data_p;
    vma_data_p = kmalloc(sizeof(this), GFP_KERNEL);
    atomic_set(&(vma_data_p->counter), 0);
    printk("%d \n", atomic_read(&(vma_data_p->counter)));

    int reg;
    reg = misc_register(&dev_handle);
    printk("minor_number: %i\n",&dev_handle.minor);
    if (reg != 0) {
        printk(KERN_ERR "Error registering misc.device for module\n");
        return reg;
    }

    printk(KERN_INFO "Paging module is loaded\n");

    return 0;
}

static void
paging_exit_mod(void)
{
    misc_deregister(&dev_handle);

    printk(KERN_INFO "Paging module is unloaded\n");
}

module_init(paging_init_mod);
module_exit(paging_exit_mod);

