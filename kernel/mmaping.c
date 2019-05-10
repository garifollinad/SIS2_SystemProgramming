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


atomic_t new_page_cnt = ATOMIC_INIT(0);
atomic_t free_page_cnt = ATOMIC_INIT(0);



//user space device interface (/dev)
//Memory areas are represented by a memory area object, which is stored in the vm_area_struct structure and defined in <linux/mm.h>
struct vma_data{
    atomic_t counter;/*number of free pages allocated, a free page has ->_count = -1.  This is so that we
     can use atomic_add_negative(-1, page->_count) to detect when the page becomes free*/
    struct page * page_address;
} this;


static int
remaping(struct vm_area_struct * vma, unsigned long addr_fault)
{
    printk("remaping, vma: %lu\n", vma);

    struct page *new_page = alloc_page(GFP_KERNEL);//alloc physic address 
    atomic_inc(&new_page_cnt);
    if(!new_page){
        return VM_FAULT_OOM;//If memory allocation fails
    } 
    unsigned long page_number = page_to_pfn(new_page);//return the page frame number associated with a struct page
    unsigned long aligned_vaddress = PAGE_ALIGN(addr_fault);
    int remaped = remap_pfn_range(vma, aligned_vaddress, page_number, PAGE_SIZE, vma->vm_page_prot);//Update the process' page tables to map the faulting virtual address to the new physical address you just allocated. 
    if(!remaped) return VM_FAULT_NOPAGE;//If everything succeeds
    printk(KERN_INFO "vma_fault_paging() invoked: took a page fault at VA 0x%lx\n", addr_fault);
    return VM_FAULT_SIGBUS;//If a failure occurs anywhere else
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int
vma_fault_paging(struct vm_area_struct * vma, struct vm_fault * vmf)
{
    unsigned long addr_fault = (unsigned long)vmf->virtual_address;
#else
static int
vma_fault_paging(struct vm_fault * vmf)

{
    struct vm_area_struct * vma = vmf->vma;
    unsigned long addr_fault = (unsigned long)vmf->address;
#endif
    printk("vma_fault_paging, vmf: %lu\n", vmf->vma);
    return remaping(vma, addr_fault);
}

static void
vma_open_paging(struct vm_area_struct * vma)
{
    printk(KERN_INFO "vma_open_paging() invoked\n");
}

static void
vma_close_paging(struct vm_area_struct * vma)
{
    atomic_inc(&free_page_cnt);
    printk("vma_close_paging, vma: %lu\n", vma);
    printk(KERN_INFO "vma_close_paging() invoked\n");
}

//mapping the new virtual address for the process 
static int
mmaping(struct file * file, struct vm_area_struct * vma)
{
    vma->vm_flags |= VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_NORESERVE
              | VM_DONTDUMP | VM_PFNMAP;// contains bit flags, defined in <linux/mm.h>, that specify the behavior of and provide information about the pages contained in the memory area.

    // setup the vma->vm_ops, so we can catch page faults 
    //points to the table of operations associated with a given memory area, which the kernel can invoke to manipulate the VMA
    vma->vm_ops = &vma_ops_paging; /*structure points to the table of operations associated with a given memory area,
     which the kernel can invoke to manipulate the VMA.*/
    printk("mmaping, vma: %lu", vma);

    printk(KERN_INFO "mmaping() invoked: new VMA for pid %d from VA 0x%lx to 0x%lx\n",
        current->pid, vma->vm_start, vma->vm_end);/*The vm_start field is the initial (lowest) address in the interval and 
    the vm_end field is the first byte after the final (highest) address in the interval.*/

    return 0;
}

static struct vm_operations_struct/*the operations table is represented by struct vm_operations_struct */
vma_ops_paging = 
{
    .fault = vma_fault_paging,
    .open  = vma_open_paging,
    .close = vma_close_paging
};

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

