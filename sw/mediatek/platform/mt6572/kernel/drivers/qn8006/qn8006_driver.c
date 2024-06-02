/* ------------------------------------------------------------------------- */
/* qn8006_driver.c - a device driver for the QN8006 FM Module interface      */
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
    d.j create 2014.11.13
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

#include <mach/mt_pm_ldo.h>

#include <linux/proc_fs.h>  /*proc*/

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>

#include "qndriver.h"


#define _DEBUG_EN_

#if defined(_DEBUG_EN_)
		
#define TRACE_FUNC        printk("[qn8006 fm] function: %s, line: %d \n", __func__, __LINE__)

#define TRACE_DEBUG(format, args...)       printk("QN8006 FM:"format,##args)
#else

#define TRACE_FUNC

#define TRACE_DEBUG(x,...)
#endif

#define QN8006_DEV_NAME "qn8006_dev"

#define SLAVE_ADDR (0x2B)

#define BUS_NUM 0
/**********************************************************************
 * Marco defined part
 *********************************************************************/
#define QN8006_CS         GPIO30
#define QN8006_DIN        GPIO31

#define QN8006_SEND_AUD   GPIO60



#define QN8006_I2C_ADDR         0x2B

#define I2C_DELAY_US             1  /* i2c speed */

#define I2C_TIME_OUT             20

#define QN8006_SCL              GPIO105
#define QN8006_SDA              GPIO106

#define I2C_SDA_MODE            mt_set_gpio_mode(QN8006_SDA,GPIO_MODE_00)
#define I2C_SCL_MODE            mt_set_gpio_mode(QN8006_SCL,GPIO_MODE_00)

#define I2C_SDA_OUTPUT          mt_set_gpio_dir(QN8006_SDA,GPIO_DIR_OUT)
#define I2C_SDA_INPUT           mt_set_gpio_dir(QN8006_SDA,GPIO_DIR_IN)
#define I2C_SCL_OUTPUT          mt_set_gpio_dir(QN8006_SCL,GPIO_DIR_OUT)

#define I2C_SDA_HIGH            mt_set_gpio_out(QN8006_SDA,GPIO_OUT_ONE)
#define I2C_SDA_LOW             mt_set_gpio_out(QN8006_SDA,GPIO_OUT_ZERO)

#define I2C_SCL_HIGH            mt_set_gpio_out(QN8006_SCL,GPIO_OUT_ONE)
#define I2C_SCL_LOW             mt_set_gpio_out(QN8006_SCL,GPIO_OUT_ZERO)

#define I2C_SDA_READ            mt_get_gpio_in(QN8006_SDA)

struct qn8006_cfg_aud{
    int optiontype;
    int option;
    int isRx;  /* 0:Tx, 1:Rx  config audio */
};

/*d.j add 2014.12.09*/
struct qn8006_getRssi{
    int ch;      /* [in]  */
    int rssi;    /* [out] */
};

struct qn8006_rxSeekCH{
    int start;
    int stop;
    int step;
    int db;
    int up;
    int ret;
};

struct qn8006_rdsLoadData_st{
    int *rdsRawData;
    int len;
    int isUpload;  /* 1:upload, 0:download */
};

struct qn8006_cfgFMMod{
    int optiontype;
    int option;
};

struct qn8006_param_fm{
    int frequency;
    int transmit;
    int power;
    int audio_path;
    struct qn8006_cfg_aud cfg_aud;
    int sys_mode;

    int cspin;
    struct qn8006_getRssi  rssi;
    struct qn8006_rxSeekCH rscall;
    struct qn8006_rxSeekCH rsc;
    int rdsen;
    int rdssignal;
    int isRDSBufReady;
    struct qn8006_rdsLoadData_st rdsld;
    int country;
    struct qn8006_cfgFMMod cfgFMod;
    int loaddftSettingCountry;
    struct qn8006_rxSeekCH clCHscan;
    int poweren;
};


/* magic number */
#define    QN8006_IOC_MAGIC                       '%'
#define    QN8006_IOC_SET_FQ                      _IOW(QN8006_IOC_MAGIC, 0x00, int)   /*FQ = Frequency*/
#define    QN8006_IOC_GET_FQ                      _IOR(QN8006_IOC_MAGIC, 0x01, int)
#define    QN8006_IOC_TRANSMIT                    _IOW(QN8006_IOC_MAGIC, 0x02, int)
#define    QN8006_IOC_SET_POWER                   _IOW(QN8006_IOC_MAGIC, 0x03, int)
#define    QN8006_IOC_GET_POWER                   _IOR(QN8006_IOC_MAGIC, 0x04, int)
#define    QN8006_IOC_AUDIO_MODE                  _IOW(QN8006_IOC_MAGIC, 0x05, int)   /*0: BLUETOOTH, 1: EARPHONE*/
#define    QN8006_IOC_AUDIO_CONFIG                _IOW(QN8006_IOC_MAGIC, 0x06, struct qn8006_cfg_aud)
#define    QN8006_IOC_SETSYS_MODE                 _IOW(QN8006_IOC_MAGIC, 0x07, int)
#define    QN8006_IOC_INIT                        _IO(QN8006_IOC_MAGIC,  0x08)
/*d.j add 2014.12.09*/
#define    QN8006_IOC_SET_CSPIN                   _IOW(QN8006_IOC_MAGIC, 0x09, int)
#define    QN8006_IOC_GET_RSSI                    _IOR(QN8006_IOC_MAGIC, 0x0a, struct qn8006_getRssi)
#define    QN8006_IOC_RXSEEKCHALL                 _IOW(QN8006_IOC_MAGIC, 0x0b, struct qn8006_rxSeekCH)
#define    QN8006_IOC_RXSEEKCH                    _IOW(QN8006_IOC_MAGIC, 0x0c, struct qn8006_rxSeekCH)
#define    QN8006_IOC_RDSENABLE                   _IOW(QN8006_IOC_MAGIC, 0x0d, int)
#define    QN8006_IOC_GET_RDSSIGNAL               _IOR(QN8006_IOC_MAGIC, 0x0e, int)
#define    QN8006_IOC_GET_ISBUFREADY              _IOR(QN8006_IOC_MAGIC, 0x0f, int)
#define    QN8006_IOC_RDSLOADDATA                 _IOW(QN8006_IOC_MAGIC, 0x10, struct qn8006_rdsLoadData_st)
#define    QN8006_IOC_SET_COUNTRY                 _IOW(QN8006_IOC_MAGIC, 0x11, int)
#define    QN8006_IOC_CFGFMMOD                    _IOW(QN8006_IOC_MAGIC, 0x12, struct qn8006_cfgFMMod)
#define    QN8006_IOC_LOADDFTSETTING              _IOW(QN8006_IOC_MAGIC, 0x13, int)
#define    QN8006_IOC_TXCLEARCHSCAN               _IOW(QN8006_IOC_MAGIC, 0x14, struct qn8006_rxSeekCH)
#define    QN8006_IOC_POWER_ENABLE                _IOW(QN8006_IOC_MAGIC, 0x15, int)



/**********************************************************************
 * All function declaration  part
 *********************************************************************/
static dev_t g_qn8006_devno;
static struct cdev *g_qn8006_cdev=NULL;
static struct class *qn8006_class = NULL;
static struct device *qn8006_nor_device = NULL;

static int qn8006_open(struct inode *inode, struct file *file);
static int qn8006_release(struct inode *inode, struct file *file);
static long qn8006_unlocked_ioctl(struct file *file, unsigned int cmd,unsigned long arg);

static int qn8006_probe(struct platform_device *pdev);
static int qn8006_remove(struct platform_device *pdev);




/**********************************************************************
 * I2c utility part
 *********************************************************************/
/* Attention:  qn8006 used nonstantard i2c protocol */
static unsigned char qnd_i2c_timeout;
static void qn8006_Set_SDA(unsigned char data)
{
    mt_set_gpio_out(QN8006_SDA, data);
}

static void qn8006_Set_SCL(unsigned char data)
{
    mt_set_gpio_out(QN8006_SCL, data);
}



static void qn8006_i2c_start(void)
{
        I2C_SDA_MODE;
	I2C_SCL_MODE;
	I2C_SDA_OUTPUT;
	I2C_SCL_OUTPUT;

        udelay(I2C_DELAY_US); 
	qn8006_Set_SCL(1);
	udelay(I2C_DELAY_US);  
	qn8006_Set_SDA(1);
	udelay(I2C_DELAY_US);  

	qn8006_Set_SDA(0);
	udelay( 2 * I2C_DELAY_US );
        qn8006_Set_SCL(0);
}

static void qn8006_i2c_stop(void)
{
	I2C_SDA_OUTPUT;
	I2C_SCL_OUTPUT;

        udelay(I2C_DELAY_US);
	qn8006_Set_SDA(0);
	udelay(I2C_DELAY_US);
	qn8006_Set_SCL(1);
	udelay( 2 * I2C_DELAY_US );
	qn8006_Set_SDA(1);
}

static void qn8006_i2c_send_ack(unsigned char i)
{
	qn8006_Set_SCL(0);
        udelay(I2C_DELAY_US);
        qn8006_Set_SDA(i);
        udelay(I2C_DELAY_US);  
	qn8006_Set_SCL(1);
        udelay(I2C_DELAY_US); 
	qn8006_Set_SCL(0);
        udelay(I2C_DELAY_US);
	qn8006_Set_SDA(1);        
}

static unsigned char qn8006_i2c_check_ack( void )
{
    unsigned char time=0;
    I2C_SDA_INPUT;
    udelay(I2C_DELAY_US);
    qn8006_Set_SCL(1);
    while(0 != I2C_SDA_READ )
    {
        udelay( 5 * I2C_DELAY_US );
        time++;
        if(time > I2C_TIME_OUT)
        {
            qn8006_Set_SCL(0);
            I2C_SDA_OUTPUT;
            return 0;
        }
    }
    //LPC_GPIO1->FIOCLR    |= LED_D4;
    udelay(I2C_DELAY_US);
    qn8006_Set_SCL(0);
    I2C_SDA_OUTPUT; 
    return 1;
}

static void qn8006_i2c_Write_Bit(unsigned char i)
{
    qn8006_Set_SCL(0);
    udelay(I2C_DELAY_US);
    qn8006_Set_SDA(i);
    udelay(I2C_DELAY_US);
    qn8006_Set_SCL(1);
    udelay( 2 * I2C_DELAY_US );
    qn8006_Set_SCL(0);
}

static void qn8006_i2c_Write_Byte(unsigned char Data)
{
    unsigned char i;
    udelay(I2C_DELAY_US);
    for(i=0;i<8;i++)
    {
        qn8006_i2c_Write_Bit(Data>>7);
        Data<<=1;
    }
}

static unsigned char qn8006_i2c_Read_Byte(void)
{
    unsigned char Data1=0x00;
    unsigned char j;
    I2C_SDA_INPUT;
    for(j=0;j<8;j++)
    {
        udelay(I2C_DELAY_US);
        qn8006_Set_SCL(1);
        Data1 = (Data1<<1) | I2C_SDA_READ ;
        udelay(I2C_DELAY_US);
        qn8006_Set_SCL(0);
    }
    I2C_SDA_OUTPUT; 
    return Data1;
}

/************************************************************************************************************
** Name: qn8006_AI2C_Write_1byte                         
** Funcation:write a data to a desired            
**           register through i2c bus 
** Description: Slave---------device address
**              Regis_Addr----register address
*************************************************************************************************************/
static unsigned char qn8006_AI2C_Write_1byte(unsigned char Slave,unsigned char Regis_Addr,unsigned char Data)
{
    unsigned char temp;
    unsigned char succ,time=0;
    temp=Slave;
    //Set_Dataout;
    qn8006_i2c_start();
    qn8006_i2c_Write_Byte(temp);
    succ=qn8006_i2c_check_ack();
    //Added to fix the I2C failure
    while((succ!=1)&&(time<10))
    {
        qn8006_i2c_stop();
        qn8006_i2c_start();
        qn8006_i2c_Write_Byte(temp);
        succ=qn8006_i2c_check_ack();
        time++;
    }
    qn8006_i2c_Write_Byte(Regis_Addr);
    qn8006_i2c_check_ack();
    qn8006_i2c_Write_Byte(Data);
    qn8006_i2c_check_ack();
    qn8006_i2c_stop();
    return succ;
}

/***********************************************************************************************************
** Name: qn8006_AI2C_Read_1byte                          
** Function: Read a data from a desired register  
**           through i2c bus 
** Description: Slave---------device address
**              Regis_Addr----register address
************************************************************************************************************/
static unsigned char qn8006_AI2C_Read_1byte(unsigned char Slave,unsigned char Regis_Addr)
{ 
    unsigned char Data=0x00;
    unsigned char temp, succ,time=0;
    temp =Slave | 0x01;
    qn8006_i2c_start();
    qn8006_i2c_Write_Byte(temp);
    succ = qn8006_i2c_check_ack();

    while((succ!=1)&&(time<10))
    {
 	    qn8006_i2c_stop();
 	    udelay(3 * I2C_DELAY_US);
 	    qn8006_i2c_start();
 	    qn8006_i2c_Write_Byte(temp);
 	    succ=qn8006_i2c_check_ack();
 	    time++;
    }
    qn8006_i2c_Write_Byte(Regis_Addr);
    qn8006_i2c_check_ack();
    Data = qn8006_i2c_Read_Byte();
    qn8006_i2c_send_ack(1);
    qn8006_i2c_stop();
    return Data;
}

/************************************************************************************************************
** Name: qn8006_AI2C_Write_Nbyte                              
** Funcation: Write many datas to all desired registers 
** Description: Slave---------device address
**              Regis_Addr----register address
*************************************************************************************************************/
static unsigned char qn8006_AI2C_Write_Nbyte(unsigned char Slave,unsigned char Regis_Addr,unsigned char*Data,unsigned char Length)
{
    unsigned char temp,i;
    unsigned char succ,time=0;
    temp=Slave & 0xfe;
    qn8006_i2c_start();
    qn8006_i2c_Write_Byte(temp);
    succ = qn8006_i2c_check_ack();
    //Added to fix the I2C failure
    while((succ!=1)&&(time<10))
    {
        qn8006_i2c_stop();
        qn8006_i2c_start();
        qn8006_i2c_Write_Byte(temp);
        succ=qn8006_i2c_check_ack();
        time++;
    }
    qn8006_i2c_Write_Byte(Regis_Addr);
    qn8006_i2c_check_ack();
    for(i=0;i<Length;i++)
    { 
        qn8006_i2c_Write_Byte(Data[i]);
        qn8006_i2c_check_ack();
    }
    qn8006_i2c_stop();
    return succ;
}
 
/***********************************************************************************************************
** Name: qn8006_AI2C_Read_Nbyte                                
** Function:Read many datas from all desired registers
** Description: Slave---------device address
**              Regis_Addr----register address
************************************************************************************************************/
static unsigned char qn8006_AI2C_Read_Nbyte(unsigned char Slave,unsigned char Regis_Addr,unsigned char*Data,unsigned char Length)
{
    unsigned char temp,i;
    unsigned char succ,time=0;
    temp =Slave | 0x01;
    //LPC_GPIO1->FIOSET    |= LED_D4;
    I2C_SDA_OUTPUT; 
    qn8006_i2c_start();
    qn8006_i2c_Write_Byte(temp);
		
    succ=qn8006_i2c_check_ack();
	
    while((succ!=1)&&(time<10))
    {
        qn8006_i2c_stop();
 	udelay(2 * I2C_DELAY_US);
 	qn8006_i2c_start();
 	qn8006_i2c_Write_Byte(temp);
 	succ=qn8006_i2c_check_ack();
 	time++;
    }
    //LPC_GPIO1->FIOSET    |= LED_D4;
    qn8006_i2c_Write_Byte(Regis_Addr);
    qn8006_i2c_check_ack();
    I2C_SDA_INPUT;
    for(i=0;i<Length;i++) { 
        Data[i] = qn8006_i2c_Read_Byte();
        if(i==Length-1)
       	    qn8006_i2c_send_ack(1);
        else
            qn8006_i2c_send_ack(0);
    }
    qn8006_i2c_stop();
    return succ;
}

/************************************************************************************************************
** Name: qn8006_ChipReset                       
** Funcation: Reset chip working state          
**            through i2c bus 
*************************************************************************************************************/
static unsigned char qn8006_ChipReset(unsigned char Slave)	  //for 2-wire only
{
    unsigned char temp;
    unsigned char i;
    temp=Slave;
    I2C_SDA_OUTPUT; 
    qn8006_i2c_start();
    qn8006_i2c_Write_Byte(temp);
    qn8006_i2c_check_ack();
    if(!qnd_i2c_timeout) {
        qn8006_i2c_Write_Byte(0x01);
        qn8006_i2c_check_ack();
        if(!qnd_i2c_timeout) {
            qn8006_i2c_Write_Byte(0x80);
        }
    }
    
    for(i=0;i<4;i++)
    {
       udelay(2 * I2C_DELAY_US);
    }
    return !qnd_i2c_timeout;
}

static unsigned char QND_WRITE_N(unsigned char Regis_Addr,unsigned char *Data,unsigned char Length)
{
    return qn8006_AI2C_Write_Nbyte((QN8006_I2C_ADDR<<1), Regis_Addr,Data,Length);
}

static unsigned char QND_READ_N(unsigned char Regis_Addr,unsigned char *Data,unsigned char Length)
{
    return qn8006_AI2C_Read_Nbyte((QN8006_I2C_ADDR<<1), Regis_Addr,Data,Length);
}


/************************************************************************************************************
** Name: qn8006_gpio_i2c_ReadReg                      
** Funcation: qn8006 i2c read API          
**            through i2c bus 
*************************************************************************************************************/
unsigned char qn8006_gpio_i2c_ReadReg(unsigned char addr)
{
	unsigned char ret=0;
	QND_READ_N(addr,&ret,1);	
	return ret;
}
EXPORT_SYMBOL(qn8006_gpio_i2c_ReadReg);

/************************************************************************************************************
** Name: qn8006_gpio_i2c_WriteReg                      
** Funcation: qn8006 i2c write API          
**            through i2c bus 
*************************************************************************************************************/
unsigned char qn8006_gpio_i2c_WriteReg(unsigned char addr, unsigned char value)
{
	unsigned char ret=0;
	QND_WRITE_N(addr,&value,1);
	return ret;
}
EXPORT_SYMBOL(qn8006_gpio_i2c_WriteReg);

/**********************************************************************
int QND_Delay() for qndriver.c call
**********************************************************************
Description: Delay for some ms, to be customized according to user
             application platform

Parameters:
        ms: ms counts
Return Value:
        None
            
**********************************************************************/
void QND_Delay(unsigned short ms)
{
        msleep(ms);
}
EXPORT_SYMBOL(QND_Delay);


static void qn8006_i2c_initial(void)
{
        I2C_SDA_MODE;
	I2C_SCL_MODE;
	I2C_SDA_OUTPUT;
	I2C_SCL_OUTPUT;

        mt_set_gpio_pull_enable(QN8006_SDA, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(QN8006_SCL, GPIO_PULL_DISABLE);

	I2C_SDA_HIGH;
	I2C_SCL_HIGH;
}

static void qn8006_power_enable(unsigned char en)
{
        if(en)
        {/*power on*/
             /*provide 3.3v voltage*/
             hwPowerOn(MT6323_POWER_LDO_VCAMA, VOL_3300,"qn8006_dev");
        }
        else
        {/*power off*/
             hwPowerDown(MT6323_POWER_LDO_VCAMA, "qn8006_dev");
        }
}



/**********************************************************************
 * QN8006 Driver part
 *********************************************************************/
static const struct file_operations g_qn8006_fops = 
{
    .owner = THIS_MODULE,
    .open = qn8006_open,
    .release = qn8006_release,
    .unlocked_ioctl = qn8006_unlocked_ioctl,
};

static int qn8006_open(struct inode *inode, struct file *file)
{
    TRACE_FUNC;
    return 0;
}

static int qn8006_release(struct inode *inode, struct file *file)
{
    TRACE_FUNC;
    return 0;
}

static long qn8006_unlocked_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
    struct qn8006_param_fm qn_fm;

    TRACE_FUNC;

    switch(cmd)
    {
        case QN8006_IOC_SET_FQ:
             if(copy_from_user(&qn_fm.frequency, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_SET_FQ-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_SET_FQ:%s,%d qn_fm.frequency=%d\n", __func__, __LINE__, qn_fm.frequency);
             QND_TuneToCH( (unsigned short)qn_fm.frequency );
            break;

        case QN8006_IOC_GET_FQ:
             qn_fm.frequency = QNF_GetCh();
             if(copy_to_user((void *)arg, &qn_fm.frequency, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_GET_FQ-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
            break;

        case QN8006_IOC_TRANSMIT:
            break;

        case QN8006_IOC_SET_POWER:
             if(copy_from_user(&qn_fm.power, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_SET_POWER-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_SET_POWER:%s,%d qn_fm.power=%d\n", __func__, __LINE__, qn_fm.power);
             QND_TXSetPower( (unsigned char)qn_fm.power );
            break;

        case QN8006_IOC_GET_POWER:
             if(copy_to_user((void *)arg, &qn_fm.power, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_GET_POWER-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
            break;

        case QN8006_IOC_AUDIO_MODE:
             if(copy_from_user(&qn_fm.audio_path, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_AUDIO_MODE-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_AUDIO_MODE:%s,%d qn_fm.audio_path=%d\n", __func__, __LINE__, qn_fm.audio_path);
             mt_set_gpio_mode(QN8006_SEND_AUD,GPIO_MODE_00);
             mt_set_gpio_dir(QN8006_SEND_AUD,GPIO_DIR_OUT);
             mt_set_gpio_out(QN8006_SEND_AUD,qn_fm.audio_path);
            break;

        case QN8006_IOC_AUDIO_CONFIG:
             if(copy_from_user(&qn_fm.cfg_aud, (void *)arg, sizeof(struct qn8006_cfg_aud)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_AUDIO_CONFIG-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_AUDIO_CONFIG:%s,%d qn_fm.cfg_aud.isRx=%d, qn_fm.cfg_aud.optiontype=%d, qn_fm.cfg_aud.option=%d\n", __func__, __LINE__, qn_fm.cfg_aud.isRx, qn_fm.cfg_aud.optiontype, qn_fm.cfg_aud.option);
              
             if(qn_fm.cfg_aud.isRx)
             { /* RxConfigAudio */
                 QND_RXConfigAudio((unsigned char)qn_fm.cfg_aud.optiontype , (unsigned char)qn_fm.cfg_aud.option);
             }
             else
             { /* TxConfigAudio */
                 QND_TXConfigAudio((unsigned char)qn_fm.cfg_aud.optiontype , (unsigned char)qn_fm.cfg_aud.option);
             }
            break;

        case QN8006_IOC_SETSYS_MODE:
             if(copy_from_user(&qn_fm.sys_mode, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_AUDIO_CONFIG-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_AUDIO_CONFIG:%s,%d qn_fm.sys_mode=%d\n", __func__, __LINE__, qn_fm.sys_mode);
             QND_SetSysMode( (unsigned short)qn_fm.sys_mode );
            break;

        case QN8006_IOC_INIT:
             QND_Init();
            break;

        case QN8006_IOC_SET_CSPIN:
             if(copy_from_user(&qn_fm.cspin, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_SET_CSPIN-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_SET_CSPIN:%s,%d qn_fm.cspin=%d\n", __func__, __LINE__, qn_fm.cspin);
             mt_set_gpio_mode(QN8006_CS,GPIO_MODE_00);
             mt_set_gpio_dir(QN8006_CS,GPIO_DIR_OUT);
             mt_set_gpio_out(QN8006_CS,qn_fm.cspin);
            break;

        case QN8006_IOC_GET_RSSI:
             if(copy_from_user(&qn_fm.rssi, (void *)arg, sizeof(struct qn8006_getRssi)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_GET_RSSI-[1]-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             
             qn_fm.rssi.rssi = QND_GetRSSI((unsigned short)qn_fm.rssi.ch);
             TRACE_DEBUG(" QN8006_IOC_GET_RSSI:%s,%d qn_fm.rssi.ch=%d, qn_fm.rssi.rssi=%d\n", __func__, __LINE__, qn_fm.rssi.ch, qn_fm.rssi.rssi);
        
             if(copy_to_user((void *)arg, &qn_fm.rssi, sizeof(struct qn8006_getRssi)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_GET_RSSI-[2]-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
            break;

        case QN8006_IOC_RXSEEKCHALL:
             if(copy_from_user(&qn_fm.rscall, (void *)arg, sizeof(struct qn8006_rxSeekCH)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_RXSEEKCHALL-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_RXSEEKCHALL:%s,%d start=%d, stop=%d, step=%d, db=%d, up=%d\n", __func__, __LINE__, qn_fm.rscall.start, qn_fm.rscall.stop, qn_fm.rscall.step, qn_fm.rscall.db, qn_fm.rscall.up);
             qn_fm.rscall.ret = QND_RXSeekCHAll((unsigned short)qn_fm.rscall.start, (unsigned short)qn_fm.rscall.stop, (unsigned char)qn_fm.rscall.step, (unsigned char)qn_fm.rscall.db, (unsigned char)qn_fm.rscall.up);

             TRACE_DEBUG(" QN8006_IOC_RXSEEKCHALL:%s,%d qn_fm.rscall.ret=%d\n", __func__, __LINE__, qn_fm.rscall.ret);

             if(copy_to_user((void *)arg, &qn_fm.rscall, sizeof(struct qn8006_rxSeekCH)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_RXSEEKCHALL-[2]-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
            break;

        case QN8006_IOC_RXSEEKCH:
             if(copy_from_user(&qn_fm.rsc, (void *)arg, sizeof(struct qn8006_rxSeekCH)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_RXSEEKCH-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_RXSEEKCH:%s,%d start=%d, stop=%d, step=%d, db=%d, up=%d\n", __func__, __LINE__, qn_fm.rsc.start, qn_fm.rsc.stop, qn_fm.rsc.step, qn_fm.rsc.db, qn_fm.rsc.up);
             qn_fm.rsc.ret = QND_RXSeekCH((unsigned short)qn_fm.rsc.start, (unsigned short)qn_fm.rsc.stop, (unsigned char)qn_fm.rsc.step, (unsigned char)qn_fm.rsc.db, (unsigned char)qn_fm.rsc.up);

             TRACE_DEBUG(" QN8006_IOC_RXSEEKCH:%s,%d qn_fm.rsc.ret=%d\n", __func__, __LINE__, qn_fm.rsc.ret);

             if(copy_to_user((void *)arg, &qn_fm.rsc, sizeof(struct qn8006_rxSeekCH)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_RXSEEKCH-[2]-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
            break;

        case QN8006_IOC_RDSENABLE:
             if(copy_from_user(&qn_fm.rdsen, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_RDSENABLE-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_RDSENABLE:%s,%d qn_fm.rdsen=%d\n", __func__, __LINE__, qn_fm.rdsen);
             QND_RDSEnable( (unsigned char)qn_fm.rdsen );
            break;

        case QN8006_IOC_GET_RDSSIGNAL:
             qn_fm.rdssignal = QND_RDSDetectSignal();
             TRACE_DEBUG(" QN8006_IOC_GET_RDSSIGNAL:%s,%d qn_fm.rdssignal=%d\n", __func__, __LINE__, qn_fm.rdssignal);
             if(copy_to_user((void *)arg, &qn_fm.rdssignal, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_GET_RDSSIGNAL-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
            break;

        case QN8006_IOC_GET_ISBUFREADY:
             qn_fm.isRDSBufReady = QND_RDSCheckBufferReady();
             TRACE_DEBUG(" QN8006_IOC_GET_ISBUFREADY:%s,%d qn_fm.isRDSBufReady=%d\n", __func__, __LINE__, qn_fm.isRDSBufReady);
             if(copy_to_user((void *)arg, &qn_fm.isRDSBufReady, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_GET_ISBUFREADY-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             } 
            break;           

        case QN8006_IOC_RDSLOADDATA:
             if(copy_from_user(&qn_fm.rdsld, (void *)arg, sizeof(struct qn8006_rdsLoadData_st)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_RDSLOADDATA-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG("data address: qn_fm.rdsld.rdsRawData=0x%x \n", qn_fm.rdsld.rdsRawData);
             TRACE_DEBUG(" QN8006_IOC_RDSLOADDATA:%s,%d d[0]=%d, d[1]=%d, d[2]=%d, d[3]=%d, d[4]=%d, d[5]=%d, d[6]=%d, d[7]=%d\n", __func__, __LINE__, qn_fm.rdsld.rdsRawData[0], qn_fm.rdsld.rdsRawData[1], qn_fm.rdsld.rdsRawData[2], qn_fm.rdsld.rdsRawData[3], qn_fm.rdsld.rdsRawData[4], qn_fm.rdsld.rdsRawData[5], qn_fm.rdsld.rdsRawData[6], qn_fm.rdsld.rdsRawData[7]);
             QND_RDSLoadData( qn_fm.rdsld.rdsRawData, (unsigned char)qn_fm.rdsld.isUpload ); /* 1:upload, 0:download */
            break;

        case QN8006_IOC_SET_COUNTRY:
             if(copy_from_user(&qn_fm.country, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_SET_COUNTRY-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_SET_COUNTRY:%s,%d qn_fm.country=%d\n", __func__, __LINE__, qn_fm.country);
             QND_SetCountry( (unsigned char)qn_fm.country );
            break;

        case QN8006_IOC_CFGFMMOD:
             if(copy_from_user(&qn_fm.cfgFMod, (void *)arg, sizeof(struct qn8006_cfgFMMod)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_CFGFMMOD-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_CFGFMMOD:%s,%d qn_fm.cfgFMod.optiontype=%d, qn_fm.cfgFMod.option=%d\n", __func__, __LINE__, qn_fm.cfgFMod.optiontype, qn_fm.cfgFMod.option);
             QND_ConfigFMModule( (unsigned char)qn_fm.cfgFMod.optiontype, (unsigned char)qn_fm.cfgFMod.option );
            break;

        case QN8006_IOC_LOADDFTSETTING:
             if(copy_from_user(&qn_fm.loaddftSettingCountry, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_SET_COUNTRY-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_SET_COUNTRY:%s,%d qn_fm.loaddftSettingCountry=%d\n", __func__, __LINE__, qn_fm.loaddftSettingCountry);
             QND_LoadDefalutSetting( (unsigned char)qn_fm.loaddftSettingCountry );
            break;

        case QN8006_IOC_TXCLEARCHSCAN:
             if(copy_from_user(&qn_fm.clCHscan, (void *)arg, sizeof(struct qn8006_rxSeekCH)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_TXCLEARCHSCAN-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_TXCLEARCHSCAN:%s,%d start=%d, stop=%d, step=%d, db=%d\n", __func__, __LINE__, qn_fm.clCHscan.start, qn_fm.clCHscan.stop, qn_fm.clCHscan.step, qn_fm.clCHscan.db);
             qn_fm.clCHscan.ret = QND_TXClearChannelScan((unsigned short)qn_fm.clCHscan.start, (unsigned short)qn_fm.clCHscan.stop, (unsigned char)qn_fm.clCHscan.step, (unsigned char)qn_fm.clCHscan.db);

             TRACE_DEBUG(" QN8006_IOC_TXCLEARCHSCAN:%s,%d qn_fm.clCHscan.ret=%d\n", __func__, __LINE__, qn_fm.clCHscan.ret);

             if(copy_to_user((void *)arg, &qn_fm.clCHscan, sizeof(struct qn8006_rxSeekCH)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_TXCLEARCHSCAN-[2]-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
            break;

        case QN8006_IOC_POWER_ENABLE:
             if(copy_from_user(&qn_fm.poweren, (void *)arg, sizeof(int)) != 0 )
             {
                 TRACE_DEBUG(" QN8006_IOC_POWER_ENABLE-err:%s,%d \n", __func__, __LINE__);
                 goto fault;
             }
             TRACE_DEBUG(" QN8006_IOC_POWER_ENABLE:%s,%d qn_fm.poweren=%d\n", __func__, __LINE__, qn_fm.poweren);
             qn8006_power_enable( (unsigned char)qn_fm.poweren );
            break;
    }

    return 0;

fault:
    return -EFAULT;
}




static struct platform_driver qn8006_driver = {
	.probe	= qn8006_probe,
	.remove  = qn8006_remove,
	.driver    = {
	.name       = "QN8006_Driver",
        .owner      = THIS_MODULE,
	},
};




static int qn8006_probe(struct platform_device *pdev)
{
    int ret = 0;
    unsigned char qn8006_id1;
    unsigned char qn8006_id2;
    unsigned char qn8006_id3;

    TRACE_FUNC;
    
    ret = alloc_chrdev_region(&g_qn8006_devno, 0, 1, QN8006_DEV_NAME);
    if(ret)
    {
        TRACE_DEBUG(" ERR: allocate device Major number failed!");
    }

    g_qn8006_cdev = cdev_alloc();
    if(NULL == g_qn8006_cdev)
    {        
        TRACE_DEBUG(" ERR: allocate character device mem failed!");
        unregister_chrdev_region( g_qn8006_devno, 1 );
        return -ENOMEM;
    }
    
    /* Attatch file operation. */
    cdev_init(g_qn8006_cdev, &g_qn8006_fops);
    g_qn8006_cdev->owner = THIS_MODULE;

    ret = cdev_add(g_qn8006_cdev, g_qn8006_devno, 1);
    if(ret)
    {
        TRACE_DEBUG(" ERR: attach file operation to device failed!");
        unregister_chrdev_region( g_qn8006_devno, 1 );
        return -ENOMEM;
    }

   qn8006_class = class_create(THIS_MODULE, QN8006_DEV_NAME);
   if (IS_ERR(qn8006_class))
   {
        ret = PTR_ERR(qn8006_class);
        TRACE_DEBUG(" ERR:Unable to create class, err = %d\n", ret);
        return ret;
   }

    // if we want auto creat device node, we must call this
   qn8006_nor_device = device_create(qn8006_class, NULL, g_qn8006_devno, NULL, QN8006_DEV_NAME); 

    qn8006_i2c_initial();

#if 0 /* d.j add here for test */
    /* 1. power on */
    hwPowerOn(MT6323_POWER_LDO_VCAMA, VOL_3300,"qn8006_dev"); /*provide 3.3v voltage*/

    /* 2. set cs pin high */
    mt_set_gpio_mode(QN8006_CS,GPIO_MODE_00);
    mt_set_gpio_dir(QN8006_CS,GPIO_DIR_OUT);
    mt_set_gpio_out(QN8006_CS,GPIO_OUT_ONE);

    /* 3. initialize FM */
    QND_Init();


    /* 4. set system mode*/
    QND_SetSysMode(QND_MODE_TX|QND_MODE_FM);

    /* 5. set tune to ch : [in] 8800 */
    QND_TuneToCH(8800);

    /* 6. set TX power : [in] 0*/
    QND_TXSetPower(0);

    /* 7. set audio path : GPIO60 */
    mt_set_gpio_mode(QN8006_SEND_AUD,GPIO_MODE_00);
    mt_set_gpio_dir(QN8006_SEND_AUD,GPIO_DIR_OUT);
    //mt_set_gpio_out(QN8006_SEND_AUD,GPIO_OUT_ONE);
    mt_set_gpio_out(QN8006_SEND_AUD,GPIO_OUT_ZERO);
#endif

    
    return ret;
}

static int qn8006_remove(struct platform_device *pdev)
{
     int ret = 0;
     TRACE_FUNC;
     mt_set_gpio_out(QN8006_SEND_AUD,GPIO_OUT_ZERO);
     mt_set_gpio_out(QN8006_CS,GPIO_OUT_ZERO);
     hwPowerDown(MT6323_POWER_LDO_VCAMA, "qn8006_dev");

     device_del(qn8006_nor_device);
     class_destroy(qn8006_class);
     unregister_chrdev_region( g_qn8006_devno, 1 );
     cdev_del( g_qn8006_cdev );
     return ret;
}

static int __init qn8006_init( void )
{
    int ret = 0;
    TRACE_FUNC;
    ret = platform_driver_register(&qn8006_driver);
    return ret;
}

static void __exit qn8006_exit( void )
{

    TRACE_FUNC;
    platform_driver_unregister(&qn8006_driver);

}

module_init(qn8006_init);
module_exit(qn8006_exit);
MODULE_DESCRIPTION("QN8006 FM DEVICE driver");
MODULE_AUTHOR("dingjun <dingj@leatek.com.cn>");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("qn8006 fm device");

