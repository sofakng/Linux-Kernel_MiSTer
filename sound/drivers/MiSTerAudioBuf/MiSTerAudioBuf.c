#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h> 
#include <linux/module.h>
#include <linux/seq_file.h> 
#include <asm/uaccess.h>
#define NAME "MrAudioBuffer"

static int major = -1;
static struct cdev mycdev;
static struct class *myclass = NULL;

/////////////////////////////////////////////////////////////////////////////////////////
//
// this is the kernel-space ring buffer to hold the raw audio samples.
//
#define VERSION "Version .6a BBond007"
#define MR_BUFFER_LEN 512 * 1000
typedef struct RingBuffer
{
    unsigned int rate;
    unsigned int len;
    unsigned int index;
    char buf [MR_BUFFER_LEN];
} RingBuffer_t;

static RingBuffer_t MrBuffer;
static char msg[1024];          // The msg the device will give when asked */
static char *msg_Ptr;
static int  MrBufferWriteCount  = 0;
static int  MrBufferMaxWriteLen = 0;

/////////////////////////////////////////////////////////////////////////////////////////
//
// static int show(struct seq_file *m, 
//                 void *v)
//
static int show(struct seq_file *m, void *v)
{
    seq_printf(m, "TEST-->");
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//  static int device_open(struct inode *inode, 
//                         struct file *file)
//
static int device_open(struct inode *inode, struct file *file)
{
    sprintf(msg,
                "0x%08x|%d|%d|%d|%d\n"  
                "MrBuffer Address          --> 0x%08x\n"
                "MrBuffer Size             --> %d\n"
                "MrBuffer Index            --> %d\n"
                "MrBuffer Write Count      --> %d\n"
                "MrBuffer Max Write Length --> %d\n"
                VERSION "\n", 
                (unsigned int) &MrBuffer,
                MrBuffer.len,
                MrBuffer.index,
                MrBufferWriteCount,
                MrBufferMaxWriteLen,
                (unsigned int) &MrBuffer,
                MrBuffer.len,
                MrBuffer.index,
                MrBufferWriteCount,
                MrBufferMaxWriteLen);
    //(MrBuffer.index < 80)?&MrBuffer.buf[0]:"");
    msg_Ptr = msg;

    return single_open(file, show, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//  static ssize_t device_read(struct file *filp,  <--  see include/linux/fs.h  
//                             char *buffer,       <--  buffer to fill with data
//                             size_t length,      <--  length of the buffer    
//                             loff_t * offset)
//
static ssize_t device_read(struct file *filp,   
                           char *buffer,        
                           size_t length,       
                           loff_t * offset)
{
    
     
    int bytes_read = 0; // <-- Number of bytes actually written to the buffer

    if (*msg_Ptr == 0)  // If we're at the end of the message, 
        return 0;       // return 0 signifying end of file

    while (length && *msg_Ptr) // Actually put the data into the buffer
    {
          // The buffer is in the user data segment, not the kernel
          // segment so "*" assignment won't work.  We have to use
          // put_user which copies data from the kernel data segment to
          // the user data segment.
        put_user(*(msg_Ptr++), buffer++);
        length--;
        bytes_read++;
    }

    return bytes_read;  // Most read functions return the number of 
                        // bytes put into the buffer 
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// static ssize_t device_write(struct file *filp,
//                            const char *userBuf,
//                            size_t userBufLen,
//                            loff_t * off)
//
static ssize_t device_write(struct file *filp,
                            const char *userBuf,
                            size_t userBufLen,
                            loff_t * off)
{
    MrBufferWriteCount++;
    if(userBufLen > MrBufferMaxWriteLen)
        MrBufferMaxWriteLen = userBufLen;

    if(userBufLen > MR_BUFFER_LEN) 
        return -EFAULT;

    return userBufLen;
    
    if (userBufLen + MrBuffer.index <=  MR_BUFFER_LEN)
    {
        if(copy_from_user(&MrBuffer.buf[MrBuffer.index], userBuf, userBufLen)) return -EFAULT;
        MrBuffer.index += userBufLen;
    }
    else
    {
        int split = MR_BUFFER_LEN - MrBuffer.index;
        int remainder = userBufLen - split;
        if(copy_from_user(&MrBuffer.buf[MrBuffer.index], userBuf, split)) return -EFAULT;
        if(copy_from_user(&MrBuffer.buf[0], &userBuf[split], remainder))  return -EFAULT;
        MrBuffer.index = remainder;
    }
    return userBufLen;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// static const struct file_operations fops
//
static const struct file_operations fops =
{
    .open = device_open,
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .release = single_release,
};


/////////////////////////////////////////////////////////////////////////////////////////
//
//  static void cleanup(int device_created)
//
static void cleanup(int device_created)
{
    if (device_created)
    {
        device_destroy(myclass, major);
        cdev_del(&mycdev);
    }
    if (myclass)
        class_destroy(myclass);
    if (major != -1)
        unregister_chrdev_region(major, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//  static int device_init(void)
//
static int device_init(void)
{
    int device_created = 0;
    //init the ring buffer.
    MrBuffer.rate = 48000;
    MrBuffer.len = MR_BUFFER_LEN;
    // cat /proc/devices 
    if (alloc_chrdev_region(&major, 0, 1, NAME "_proc") < 0)
    {
        printk(KERN_INFO "MrBuffer ERROR:--> alloc_chrdev_region(%d, '%s')\n", 
               major, NAME "_proc");
        goto error;
    }
    // ls /sys/class 
    if ((myclass = class_create(THIS_MODULE, NAME "_sys")) == NULL)
    {
        printk(KERN_INFO "MrBuffer ERROR:--> class_create('%s')\n", 
               NAME "_SYS");
        goto error;
    }
    // ls /dev/ 
    if (device_create(myclass, NULL, major, NULL, NAME) == NULL)
    {
        printk(KERN_INFO "MrBuffer ERROR:--> device_create('%s', %d, '%s')\n", 
               myclass->name, 
               major, 
               NAME);
        goto error;
    }
    device_created = 1;
    cdev_init(&mycdev, &fops);
    if (cdev_add(&mycdev, major, 1) == -1)
    {
        printk(KERN_INFO "MrBuffer ERROR:--> cdev_add(%d)\n", major);
        goto error;
    }
    printk(KERN_INFO "MrBuffer Audio buffer '/dev/%s' created.\n"
                     "Class --> '%s'\n"
                     "Major --> %d\n", 
                     NAME,
                     myclass->name,  
                     major);
    return 0;
error:
    cleanup(device_created);
    return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//  static void device_exit(void)
//
static void device_exit(void)
{
    cleanup(1);
}

module_init(device_init)
module_exit(device_exit)
MODULE_LICENSE("GPL");