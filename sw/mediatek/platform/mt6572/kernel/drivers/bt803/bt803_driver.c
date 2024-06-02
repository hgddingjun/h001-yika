/* ------------------------------------------------------------------------- */
/* bt803_driver.c - a device driver for the BT803 Module interface           */
/* ------------------------------------------------------------------------- */
/*   
    Copyright (C) 2014-2020 Leatek Co.,Ltd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    d.j create 2014.11.05
    ver 1.0
*/
/* ------------------------------------------------------------------------- */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>

#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>

#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/time.h>

#include <linux/string.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_reg_base.h>
#include <mach/irqs.h>
#include <accdet_custom.h>
#include <accdet_custom_def.h>

#include <cust_eint.h>
#include <cust_gpio_usage.h>
#include <mach/mt_gpio.h>
#include <mach/eint.h>

#include <linux/mtgpio.h>
#include <linux/gpio.h>

#include <linux/proc_fs.h>  /*proc*/

/*----------------------------------------------------------------------
static variable defination
----------------------------------------------------------------------*/
#define BT803_DEVNAME    "bt803_dev"

#define EN_DEBUG

#if defined(EN_DEBUG)
		
#define TRACE_FUNC 	printk("[BT803] function: %s, line: %d \n", __func__, __LINE__);

#define BT803_DEBUG(format, args...)       printk("[BT803]:"format,##args)
#else

#define TRACE_FUNC

#define BT803_DEBUG(x,...)
#endif

#define  MCU_SLEEP     0
#define  MCU_WAKEUP    1

/* HALL_1 wake up mt6572 GPIO129 */
#define BT803_SWITCH_EINT        CUST_EINT_HALL_1_NUM
#define BT803_SWITCH_DEBOUNCE    CUST_EINT_HALL_1_DEBOUNCE_CN		/* ms */
#define BT803_SWITCH_TYPE        EINTF_TRIGGER_LOW
#define BT803_SWITCH_SENSITIVE   MT_LEVEL_SENSITIVE
/* HALL_1 wake up mt6572 */

#define BT803_PWR_PIN               GPIO27
#define BT803_RST_PIN               GPIO66
#define BT803_WAKEDUP_PIN           GPIO128

/****************************************************************/
/*******static function defination                             **/
/****************************************************************/
static dev_t g_bt803_devno;
static struct cdev *g_bt803_cdev = NULL;
static struct class *bt803_class = NULL;
static struct device *bt803_nor_device = NULL;
static struct input_dev *bt803_input_dev;
static struct kobject *g_bt803_sys_device;
static volatile int cur_bt803_status = MCU_WAKEUP;
struct wake_lock bt803_key_lock;
static int bt803_key_event = 0;
static int g_bt803_first = 1;

static void bt803_eint_handler(unsigned long data);
static DECLARE_TASKLET(bt803_tasklet, bt803_eint_handler, 0);

static atomic_t send_event_flag = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(send_event_wq);

/* d.j add 2014.11.06 */
static unsigned char g_ctrl_pin=0;
/*
 * g_ctrl_pin value Readme: 
 * [0x10,0x11] BT803_PWR_PIN: 0x10-->off, 0x11-->on; 
 * [0x20,0x21] BT803_RST_PIN: 0x20-->low, 0x21-->high
 * [0x30,0x31] BT803_WAKEDUP_PIN: 0x30-->low, 0x31-->high
 */
static void bt803_power_pin(bool enable);
static void bt803_reset_pin(bool enable);
static void bt803_waked_up_pin(bool enable);
static void bt803_init_all_pins(void);

static struct proc_dir_entry *pbt803_node_proc = NULL;
static ssize_t bt803_read_proc(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t bt803_write_proc(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

struct file_operations bt803_fops_proc = 
{
	.owner=THIS_MODULE,  
	.read  =  bt803_read_proc,	 
	.write =  bt803_write_proc,
};

static void bt803_init_all_pins(void)
{
    mt_set_gpio_mode(BT803_PWR_PIN,GPIO_MODE_00);
    mt_set_gpio_dir(BT803_PWR_PIN,GPIO_DIR_OUT);

    mt_set_gpio_mode(BT803_RST_PIN,GPIO_MODE_00);
    mt_set_gpio_dir(BT803_RST_PIN,GPIO_DIR_OUT);

    mt_set_gpio_mode(BT803_WAKEDUP_PIN,GPIO_MODE_00);
    mt_set_gpio_dir(BT803_WAKEDUP_PIN,GPIO_DIR_OUT);
}

static void bt803_power_pin(bool enable)
{

    mt_set_gpio_out(BT803_PWR_PIN,enable);        
}

static void bt803_reset_pin(bool enable)
{

    mt_set_gpio_out(BT803_RST_PIN,enable); 
}

static void bt803_waked_up_pin(bool enable)
{

    mt_set_gpio_out(BT803_WAKEDUP_PIN,enable);
}


static ssize_t bt803_read_proc(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
     char *ptr = buf;

     TRACE_FUNC;

     if(*f_pos)
     {
         return 0;
     }

     BT803_DEBUG(" %s: g_ctrl_pin = 0x%x\n", __func__, g_ctrl_pin);

     ptr += sprintf(ptr, "0x%02X ", g_ctrl_pin);

     ptr += sprintf(ptr, "\n");

     *f_pos += ptr-buf;

     return ( ptr-buf );
}

static ssize_t bt803_write_proc(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
     #define LIMIT_BUF   11
     S32 ret;
     S32 i;
     unsigned long val;
     unsigned char getchar_val[LIMIT_BUF]={0}; //0x12345678\0

     TRACE_FUNC;

     BT803_DEBUG(" %s: count = 0x%x\n", __func__, count);

    if(count>LIMIT_BUF)
    {
        count = LIMIT_BUF-1;
    }    

    if (copy_from_user(getchar_val, buf, count))
    {
        BT803_DEBUG("%s: copy from user fail!!!\n");
        return -EFAULT;
    }

    getchar_val[count+1] = '\0';

     for(i=0;i<LIMIT_BUF;i++)
     {
         BT803_DEBUG(" %s: getchar_val[%d]=%c \n", __func__, i, getchar_val[i]);
     }

     if(NULL != buf)
     {

         /*hex*/ /* bash shell eg: echo 10 > bt803 node (10=0x10) */
         ret = kstrtoul(getchar_val, 16, &val);
         
         BT803_DEBUG(" %s: val = 0x%x, ret = %d\n", __func__, val, ret);

         if(ret<0)
         {
              BT803_DEBUG(" %s: kstrtoul ERR!\n", __func__);
              return -EINVAL;
         }

         g_ctrl_pin = (u8)val;
     }
     else
     {
         BT803_DEBUG(" %s: ERR copy from user buffer is NULL!\n", __func__);
         return -EINVAL;
     }

     switch( g_ctrl_pin&0xF0 )
     {
        case 0x10:
          {
              BT803_DEBUG(" %s: case: bt803_power_pin: 0x%x\n", __func__, (g_ctrl_pin&0x0F) );
              bt803_power_pin( g_ctrl_pin&0x0F );
          }
          break;

        case 0x20:
          {
              BT803_DEBUG(" %s: case: bt803_reset_pin: 0x%x\n", __func__, (g_ctrl_pin&0x0F) );
              bt803_reset_pin( g_ctrl_pin&0x0F );
          }
          break;

        case 0x30:
          {
              BT803_DEBUG(" %s: case: bt803_waked_up_pin: 0x%x\n", __func__, (g_ctrl_pin&0x0F) );
              bt803_waked_up_pin( g_ctrl_pin&0x0F );
          }
          break;

#if 1 /* for debug */
        case 0x00:
          {
              BT803_DEBUG(" %s: case: bt803_close_all_pin\n", __func__ );
              bt803_power_pin( 0 );
              bt803_reset_pin( 0 );
              bt803_waked_up_pin( 0 );
          }
          break;
#endif

     }     

     return count;
}
/* d.j end 2014.11.06 */

/****************************************************************/
/*******export function defination                             **/
/****************************************************************/

static ssize_t bt803_status_info_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
	BT803_DEBUG("[bt803_dev] cur_bt803_status=%d\n", cur_bt803_status);
	return sprintf(buf, "%u\n", cur_bt803_status);
}

static ssize_t bt803_status_info_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t size)
{
	BT803_DEBUG("[bt803_dev] %s ON/OFF value = %ld:\n ", __func__, cur_bt803_status);

	if(sscanf(buf, "%u", &cur_bt803_status) != 1)
	{
		BT803_DEBUG("[bt803_dev]: Invalid values\n");
		return -EINVAL;
	}
	return size;
}

static DEVICE_ATTR(bt803_status, 0666, bt803_status_info_show,  bt803_status_info_store);

static void switch_bt803_eint_handler(void)
{
    TRACE_FUNC;
    tasklet_schedule(&bt803_tasklet);
}

static int sendKeyEvent(void *unuse)
{
    while(1)
    {
        BT803_DEBUG("[bt803_dev]:sendKeyEvent wait\n");
        //wait for signal
        wait_event_interruptible(send_event_wq, (atomic_read(&send_event_flag) != 0));

        wake_lock_timeout(&bt803_key_lock, 2*HZ);    //set the wake lock.
        BT803_DEBUG("[bt803_dev]:going to send event %d\n", bt803_key_event);

        //send key event
        if(MCU_WAKEUP == bt803_key_event)
          {
                BT803_DEBUG("[bt803_dev]:MCU_WAKEUP!\n");
                input_report_key(bt803_input_dev, KEY_WAKEUP, 1);
                input_report_key(bt803_input_dev, KEY_WAKEUP, 0);
                input_sync(bt803_input_dev);
          }
	  else if(MCU_SLEEP == bt803_key_event)
          {
                BT803_DEBUG("[bt803_dev]:MCU_SLEEP!\n");
                input_report_key(bt803_input_dev, KEY_SLEEP, 1);
                input_report_key(bt803_input_dev, KEY_SLEEP, 0);
                input_sync(bt803_input_dev);
          }
        atomic_set(&send_event_flag, 0);
    }
    return 0;
}

static ssize_t notify_sendKeyEvent(int event)
{
    bt803_key_event = event;
    atomic_set(&send_event_flag, 1);
    wake_up(&send_event_wq);
    BT803_DEBUG("[bt803_dev]:notify_sendKeyEvent !\n");
    return 0;
}

static void bt803_eint_handler(unsigned long data)
{
    u8 old_bt803_state = cur_bt803_status;
    
    TRACE_FUNC;

    mt_eint_mask(BT803_SWITCH_EINT);

   cur_bt803_status = !cur_bt803_status;

   if(cur_bt803_status)
   {
   	BT803_DEBUG("[bt803_dev]:MCU_WAKEUP \n");
	notify_sendKeyEvent(MCU_WAKEUP);
   }
   else
   {
   	BT803_DEBUG("[bt803_dev]:MCU_SLEEP \n");
	notify_sendKeyEvent(MCU_SLEEP);
   }
	
    /* for detecting the return to old_bt803_state */
    mt_eint_set_polarity(BT803_SWITCH_EINT, old_bt803_state);
    mdelay(10); 
    mt_eint_unmask(BT803_SWITCH_EINT);
}

static long bt803_unlocked_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
    BT803_DEBUG("[bt803_dev]:bt803_unlocked_ioctl \n");

    return 0;
}

static int bt803_open(struct inode *inode, struct file *file)
{ 
   	return 0;
}

static int bt803_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations g_bt803_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= bt803_unlocked_ioctl,
	.open		= bt803_open,
	.release	= bt803_release,	
};

static int bt803_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct task_struct *keyEvent_thread = NULL;

    TRACE_FUNC;

   ret = alloc_chrdev_region(&g_bt803_devno, 0, 1, BT803_DEVNAME);

  if (ret)
  {
	BT803_DEBUG("[bt803_dev]:alloc_chrdev_region: Get Major number error!\n");			
  }

  g_bt803_cdev = cdev_alloc();

  if(NULL == g_bt803_cdev)
  {
  	unregister_chrdev_region(g_bt803_devno, 1);
	BT803_DEBUG("[bt803_dev]:Allocate mem for kobject failed\n");
	return -ENOMEM;
  }

  //Attatch file operation.
  cdev_init(g_bt803_cdev, &g_bt803_fops);
  g_bt803_cdev->owner = THIS_MODULE;

  //Add to system
  ret = cdev_add(g_bt803_cdev, g_bt803_devno, 1);
  
  if(ret)
   {
   	BT803_DEBUG("[bt803_dev]:Attatch file operation failed\n");
   	unregister_chrdev_region(g_bt803_devno, 1);
	return -ENOMEM;
   }
	
   bt803_class = class_create(THIS_MODULE, BT803_DEVNAME);
   if (IS_ERR(bt803_class))
   {
        ret = PTR_ERR(bt803_class);
        BT803_DEBUG("[bt803_dev]:Unable to create class, err = %d\n", ret);
        return ret;
   }

    // if we want auto creat device node, we must call this
   bt803_nor_device = device_create(bt803_class, NULL, g_bt803_devno, NULL, BT803_DEVNAME); 

    /*Create proc*/
    pbt803_node_proc = proc_create("bt803", 0666, NULL, &bt803_fops_proc);
    if( NULL == pbt803_node_proc )
    {
        BT803_DEBUG("Create pbt803_node_proc[bt803] node Failule!");
    }
    else
    {
        BT803_DEBUG("Create pbt803_node_proc[bt803] node Successfully!");
    }
	
   bt803_input_dev = input_allocate_device();
	
   if (!bt803_input_dev)
   {
   	BT803_DEBUG("[bt803_dev]:bt803_input_dev : fail!\n");
       return -ENOMEM;
   }

   __set_bit(EV_KEY, bt803_input_dev->evbit);
   __set_bit(KEY_WAKEUP, bt803_input_dev->keybit);
   __set_bit(KEY_SLEEP , bt803_input_dev->keybit);

  bt803_input_dev->id.bustype = BUS_HOST;
  bt803_input_dev->name = "BT803_DEV";
  if(input_register_device(bt803_input_dev))
  {
	BT803_DEBUG("[bt803_dev]:bt803_input_dev register : fail!\n");
  }else
  {
	BT803_DEBUG("[bt803_dev]:bt803_input_dev register : success!!\n");
  }

   wake_lock_init(&bt803_key_lock, WAKE_LOCK_SUSPEND, "bt803 key wakelock");
  
   init_waitqueue_head(&send_event_wq);
   //start send key event thread
   keyEvent_thread = kthread_run(sendKeyEvent, 0, "keyEvent_send");
   if (IS_ERR(keyEvent_thread)) 
   { 
      ret = PTR_ERR(keyEvent_thread);
      BT803_DEBUG("[bt803_dev]:failed to create kernel thread: %d\n", ret);
   }

   if(g_bt803_first)
   {
    	g_bt803_sys_device = kobject_create_and_add("bt803_state", NULL);
    	if (g_bt803_sys_device == NULL)
	{
        	BT803_DEBUG("[bt803_dev]:%s: subsystem_register failed\n", __func__);
        	ret = -ENXIO;
        	return ret ;
    	}
	
    	ret = sysfs_create_file(g_bt803_sys_device, &dev_attr_bt803_status.attr);
    	if (ret) 
	{
        	BT803_DEBUG("[bt803_dev]:%s: sysfs_create_file failed\n", __func__);
        	kobject_del(g_bt803_sys_device);
    	}

	mt_set_gpio_mode(GPIO_HALL_1_PIN, GPIO_HALL_1_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_HALL_1_PIN, GPIO_DIR_IN);
	mt_eint_set_sens(BT803_SWITCH_EINT, BT803_SWITCH_SENSITIVE);
	mt_eint_set_hw_debounce(BT803_SWITCH_EINT, 5);
	mt_eint_registration(BT803_SWITCH_EINT, BT803_SWITCH_TYPE, switch_bt803_eint_handler, 0);
	g_bt803_first = 0;
   }

    bt803_init_all_pins();
    bt803_reset_pin(1);
    mdelay(100);


    return 0;
}

static int bt803_remove(struct platform_device *dev)	
{
	BT803_DEBUG("[bt803_dev]:bt803_remove begin!\n");
	
	device_del(bt803_nor_device);
	class_destroy(bt803_class);
	cdev_del(g_bt803_cdev);
	unregister_chrdev_region(g_bt803_devno,1);	
	input_unregister_device(bt803_input_dev);
	BT803_DEBUG("[bt803_dev]:bt803_remove Done!\n");
    
	return 0;
}

static struct platform_driver bt803_driver = {
	.probe	= bt803_probe,
	.remove  = bt803_remove,
	.driver    = {
	.name       = "BT803_Driver",
	},
};
static int __init bt803_init(void)
{
    TRACE_FUNC;
    platform_driver_register(&bt803_driver);

    return 0;
}

static void __exit bt803_exit(void)
{
    TRACE_FUNC;
    platform_driver_unregister(&bt803_driver);
}

module_init(bt803_init);
module_exit(bt803_exit);
MODULE_DESCRIPTION("BT803 DEVICE driver");
MODULE_AUTHOR("dingjun <dingj@leatek.com.cn>");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("bt803 device");

