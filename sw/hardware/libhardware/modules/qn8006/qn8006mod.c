/*
 * d.j create 2014.11.26 qn8006 module
 */
 
#define LOG_TAG "QN8006Stub"
 
#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/qn8006cdev.h>
#include <linux/ioctl.h>
 
#define DEVICE_NAME        "/dev/qn8006_dev"
#define MODULE_NAME        "qn8006_dev"
#define MODULE_AUTHOR      "dingj@leatek.com.cn"
 
 
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
 
 
/* device open and close interface */
static int qn8006_device_open(const struct hw_module_t *module, const char *name, struct hw_device_t **device);
static int qn8006_device_close(struct hw_device_t *device);
 
/* device visit interface */
static int qn8006_set_frequency(struct qn8006_device_t *dev, int  fq);
static int qn8006_get_frequency(struct qn8006_device_t *dev, int *fq);
static int qn8006_set_power(struct qn8006_device_t *dev, int  pwer);
static int qn8006_get_power(struct qn8006_device_t *dev, int *pwer);
 
static int qn8006_audio_mode(struct qn8006_device_t *dev, int audmode);
static int qn8006_audio_config(struct qn8006_device_t *dev, int optiontype, int option, int isRx);
static int qn8006_system_mode(struct qn8006_device_t *dev, int sysmode);
static int qn8006_init(struct qn8006_device_t *dev);
 
/* d.j add 2014.12.09 */
static int qn8006_csPin(struct qn8006_device_t *dev, int level);
static int qn8006_getRSSI(struct qn8006_device_t *dev, int ch);
static int qn8006_rxSeekCHAll(struct qn8006_device_t *dev, int start, int stop, int step, int db, int up);
static int qn8006_rxSeekCH(struct qn8006_device_t *dev, int start, int stop, int step, int db, int up);
static int qn8006_RDSEnable(struct qn8006_device_t *dev, int en);
static int qn8006_RDSDetectSignal(struct qn8006_device_t *dev ,int *sig);
static int qn8006_RDSCheckBufferReady(struct qn8006_device_t *dev, int *chkbufready);
static int qn8006_RDSLoadData(struct qn8006_device_t *dev, int *rawData, int len, int isUpload);
static int qn8006_setCountry(struct qn8006_device_t *dev, int country);
static int qn8006_configFMModule(struct qn8006_device_t *dev, int optiontype, int option);
static int qn8006_loadDefalutSetting(struct qn8006_device_t *dev, int country);
static int qn8006_txClearChannelScan(struct qn8006_device_t *dev, int start, int stop, int step, int db);
static int qn8006_powerEnable(struct qn8006_device_t *dev, int en);
 
 
/* module method table */
static struct hw_module_methods_t qn8006_module_methods={
    open: qn8006_device_open
};
 
/* module instants variable */
struct qn8006_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: QN8006_HARDWARE_MODULE_ID,
        name: MODULE_NAME,
        author: MODULE_AUTHOR,
        methods: &qn8006_module_methods,
    }
};
 
 
static int qn8006_device_open(const struct hw_module_t *module, const char *name, struct hw_device_t **device)
{
    struct qn8006_device_t *dev;
    dev = (struct qn8006_device_t *)malloc(sizeof(struct qn8006_device_t));
     
    ALOGI("Qn8006 Stub: enter qn8006_device_open function!");
 
    if(NULL == dev)
    {
        ALOGE("Qn8006 Stub: fail to alloc memory!");
    }
 
    memset(dev, 0, sizeof(struct qn8006_device_t));
    dev->common.tag = HARDWARE_MODULE_TAG;
    dev->common.version = 0;
    dev->common.module = (hw_module_t *)module;
    dev->common.close = qn8006_device_close;
     
    dev->set_frequency = qn8006_set_frequency;
    dev->get_frequency = qn8006_get_frequency;
 
    dev->set_power = qn8006_set_power;
    dev->get_power = qn8006_get_power;
 
    dev->audio_mode = qn8006_audio_mode;
    dev->audio_config = qn8006_audio_config;
    dev->system_mode = qn8006_system_mode;
    dev->init = qn8006_init;
 
    /* d.j add 2014.12.09 */
    dev->csPin = qn8006_csPin;
    dev->getRSSI = qn8006_getRSSI;
    dev->rxSeekCHAll = qn8006_rxSeekCHAll;
    dev->rxSeekCH = qn8006_rxSeekCH;
    dev->RDSEnable = qn8006_RDSEnable;
    dev->RDSDetectSignal = qn8006_RDSDetectSignal;
    dev->RDSCheckBufferReady = qn8006_RDSCheckBufferReady;
    dev->RDSLoadData = qn8006_RDSLoadData;
    dev->setCountry = qn8006_setCountry;
    dev->configFMModule = qn8006_configFMModule;
    dev->loadDefalutSetting = qn8006_loadDefalutSetting;
    dev->txClearChannelScan = qn8006_txClearChannelScan;
    dev->powerEnable = qn8006_powerEnable;
 
    if( -1 == (dev->fd=open(DEVICE_NAME, O_RDWR)) )
    {
        ALOGE("Qn8006 Stub: fail to open /dev/qn8006_dev --- %s", strerror(errno));
        free(dev);
        return -EFAULT;
    }
     
    *device = &(dev->common);
 
    ALOGI("Qn8006 Stub: open /dev/qn8006_dev successfully!");
 
    return 0;
}
 
static int qn8006_device_close(struct hw_device_t *device)
{
    struct qn8006_device_t *qn_dev = (struct qn8006_device_t *)device;
 
    ALOGI("Qn8006 Stub: enter qn8006_device_close function!");
    if( qn_dev )
    {
        close( qn_dev->fd );
        free( qn_dev );
    }
    return 0;
}
 
static int qn8006_set_frequency(struct qn8006_device_t *dev, int  fq)
{
    int err;
 
    ALOGI("Qn8006 Stub: enter qn8006_set_frequency function!");
    err = ioctl(dev->fd, QN8006_IOC_SET_FQ, &fq);
    if(err)
    {
        ALOGE(" QN8006_IOC_SET_FQ failed --> %s", strerror(errno));
    }   
    return 0;
}
 
static int qn8006_get_frequency(struct qn8006_device_t *dev, int *fq)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_get_frequency function!");
    err = ioctl(dev->fd, QN8006_IOC_GET_FQ, fq);
    if(err)
    {
        ALOGE(" QN8006_IOC_GET_FQ failed --> %s", strerror(errno));
    }  
    return 0;
}
 
static int qn8006_set_power(struct qn8006_device_t *dev, int  pwer)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_set_power function!");
    err = ioctl(dev->fd, QN8006_IOC_SET_POWER, &pwer);
    if(err)
    {
        ALOGE(" QN8006_IOC_SET_POWER failed --> %s", strerror(errno));
    }  
    return 0;
}
 
static int qn8006_get_power(struct qn8006_device_t *dev, int *pwer)
{
    int err;
    ALOGI("Qn8006 Stub: qn8006_get_power function!");
    err = ioctl(dev->fd, QN8006_IOC_GET_POWER, pwer);
    if(err)
    {
        ALOGE(" QN8006_IOC_GET_POWER failed --> %s", strerror(errno));
    } 
    return 0;
}
 
 
static int qn8006_audio_mode(struct qn8006_device_t *dev, int audmode)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_audio_mode function!");
    err = ioctl(dev->fd, QN8006_IOC_AUDIO_MODE, &audmode);
    if(err)
    {
        ALOGE(" QN8006_IOC_AUDIO_MODE failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_audio_config(struct qn8006_device_t *dev, int optiontype, int option, int isRx)
{
    int err;
    struct qn8006_cfg_aud cfg;
    cfg.optiontype = optiontype;
    cfg.option = option;
    cfg.isRx = isRx;
 
    ALOGI("Qn8006 Stub: enter qn8006_audio_config function!");
    err = ioctl(dev->fd, QN8006_IOC_AUDIO_CONFIG, &cfg);
    if(err)
    {
        ALOGE(" QN8006_IOC_AUDIO_CONFIG failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_system_mode(struct qn8006_device_t *dev, int sysmode)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_system_mode function!");
    err = ioctl(dev->fd, QN8006_IOC_SETSYS_MODE, &sysmode);
    if(err)
    {
        ALOGE(" QN8006_IOC_SETSYS_MODE failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_init(struct qn8006_device_t *dev)
{
    int err;
    int ret = -1;
    ALOGI("Qn8006 Stub: enter qn8006_init function!");
    err = ioctl(dev->fd, QN8006_IOC_INIT, &ret);
    if(err)
    {
        ALOGE(" QN8006_IOC_INIT failed --> %s", strerror(errno));
    } 
    return 0;
}
 
 
 
/* d.j add 2014.12.09 */
static int qn8006_csPin(struct qn8006_device_t *dev, int level)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_csPin function!");
    err = ioctl(dev->fd, QN8006_IOC_SET_CSPIN, &level);
    if(err)
    {
        ALOGE(" QN8006_IOC_SET_CSPIN failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_getRSSI(struct qn8006_device_t *dev, int ch)
{
    int err;
    struct qn8006_getRssi tmpRssi;
    tmpRssi.ch = ch;
    ALOGI("Qn8006 Stub: enter qn8006_getRSSI function!");
    err = ioctl(dev->fd, QN8006_IOC_GET_RSSI, &tmpRssi);
    if(err)
    {
        ALOGE(" QN8006_IOC_GET_RSSI failed --> %s", strerror(errno));
    }
    ALOGI("Qn8006 Stub: enter qn8006_getRSSI tmpRssi.rssi=%d !", tmpRssi.rssi);
    return tmpRssi.rssi;
}
 
static int qn8006_rxSeekCHAll(struct qn8006_device_t *dev, int start, int stop, int step, int db, int up)
{
    int err;
    struct qn8006_rxSeekCH tmpRscall;
    tmpRscall.start = start;
    tmpRscall.stop  = stop;
    tmpRscall.step  = step;
    tmpRscall.db    = db;
    tmpRscall.up    = up;
 
    ALOGI("Qn8006 Stub: enter qn8006_rxSeekCHAll function!");
    err = ioctl(dev->fd, QN8006_IOC_RXSEEKCHALL, &tmpRscall);
    if(err)
    {
        ALOGE(" QN8006_IOC_RXSEEKCHALL failed --> %s", strerror(errno));
    } 
    ALOGI("Qn8006 Stub: enter qn8006_rxSeekCHAll tmpRscall.ret=%d !", tmpRscall.ret);
    return tmpRscall.ret;
}
 
static int qn8006_rxSeekCH(struct qn8006_device_t *dev, int start, int stop, int step, int db, int up)
{
    int err;
    struct qn8006_rxSeekCH tmpRsc;
    tmpRsc.start = start;
    tmpRsc.stop  = stop;
    tmpRsc.step  = step;
    tmpRsc.db    = db;
    tmpRsc.up    = up;
 
    ALOGI("Qn8006 Stub: enter qn8006_rxSeekCH function!");
    err = ioctl(dev->fd, QN8006_IOC_RXSEEKCH, &tmpRsc);
    if(err)
    {
        ALOGE(" QN8006_IOC_RXSEEKCH failed --> %s", strerror(errno));
    } 
    ALOGI("Qn8006 Stub: enter qn8006_rxSeekCH tmpRsc.ret=%d !", tmpRsc.ret);
    return tmpRsc.ret;
}
 
static int qn8006_RDSEnable(struct qn8006_device_t *dev, int en)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_RDSEnable function!");
    err = ioctl(dev->fd, QN8006_IOC_RDSENABLE, &en);
    if(err)
    {
        ALOGE(" QN8006_IOC_RDSENABLE failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_RDSDetectSignal(struct qn8006_device_t *dev ,int *sig)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_RDSDetectSignal function!");
    err = ioctl(dev->fd, QN8006_IOC_GET_RDSSIGNAL, sig);
    if(err)
    {
        ALOGE(" QN8006_IOC_GET_RDSSIGNAL failed --> %s", strerror(errno));
    } 
    return 0;
}
 
 
static int qn8006_RDSCheckBufferReady(struct qn8006_device_t *dev, int *chkbufready)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_RDSDetectSignal function!");
    err = ioctl(dev->fd, QN8006_IOC_GET_ISBUFREADY, chkbufready);
    if(err)
    {
        ALOGE(" QN8006_IOC_GET_ISBUFREADY failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_RDSLoadData(struct qn8006_device_t *dev, int *rawData, int len, int isUpload)
{
    int err;
    struct qn8006_rdsLoadData_st rdsldata;
    rdsldata.rdsRawData = rawData;
    rdsldata.len = len;
    rdsldata.isUpload   = isUpload;
 
    ALOGI("Qn8006 Stub: enter qn8006_RDSLoadData function!");
    ALOGI("Qn8006 Stub: array address rdsldata.rdsRawData=0x%x, rdsldata.len=%d, rdsldata.isUpload=%d !", rdsldata.rdsRawData, rdsldata.len, rdsldata.isUpload);
 
    ALOGI("Qn8006 Stub: d[0]=0x%x, d[1]=0x%x, d[2]=0x%x, d[3]=0x%x, d[4]=0x%x, d[5]=0x%x, d[6]=0x%x, d[7]=0x%x ", rdsldata.rdsRawData[0],rdsldata.rdsRawData[1],rdsldata.rdsRawData[2],rdsldata.rdsRawData[3],rdsldata.rdsRawData[4],rdsldata.rdsRawData[5],rdsldata.rdsRawData[6],rdsldata.rdsRawData[7]);
 
    err = ioctl(dev->fd, QN8006_IOC_RDSLOADDATA, &rdsldata);
    if(err)
    {
        ALOGE(" QN8006_IOC_RDSLOADDATA failed --> %s", strerror(errno));
    } 
 
    return 0;
}
 
static int qn8006_setCountry(struct qn8006_device_t *dev, int country)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_setCountry function!");
    err = ioctl(dev->fd, QN8006_IOC_SET_COUNTRY, &country);
    if(err)
    {
        ALOGE(" QN8006_IOC_SET_COUNTRY failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_configFMModule(struct qn8006_device_t *dev, int optiontype, int option)
{
    int err;
    struct qn8006_cfgFMMod cfgFM;
    cfgFM.optiontype = optiontype;
    cfgFM.option = option;
 
    ALOGI("Qn8006 Stub: enter qn8006_configFMModule function!");
    err = ioctl(dev->fd, QN8006_IOC_CFGFMMOD, &cfgFM);
    if(err)
    {
        ALOGE(" QN8006_IOC_CFGFMMOD failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_loadDefalutSetting(struct qn8006_device_t *dev, int country)
{
    int err;
    ALOGI("Qn8006 Stub: enter qn8006_loadDefalutSetting function!");
    err = ioctl(dev->fd, QN8006_IOC_LOADDFTSETTING, &country);
    if(err)
    {
        ALOGE(" QN8006_IOC_LOADDFTSETTING failed --> %s", strerror(errno));
    } 
    return 0;
}
 
static int qn8006_txClearChannelScan(struct qn8006_device_t *dev, int start, int stop, int step, int db)
{
    int err;
    struct qn8006_rxSeekCH tmptxclscan;
    tmptxclscan.start = start;
    tmptxclscan.stop  = stop;
    tmptxclscan.step  = step;
    tmptxclscan.db    = db;
 
    ALOGI("Qn8006 Stub: enter qn8006_txClearChannelScan function!");
    err = ioctl(dev->fd, QN8006_IOC_TXCLEARCHSCAN, &tmptxclscan);
    if(err)
    {
        ALOGE(" QN8006_IOC_TXCLEARCHSCAN failed --> %s", strerror(errno));
    } 
    ALOGI("Qn8006 Stub: enter qn8006_txClearChannelScan tmptxclscan.ret=%d !", tmptxclscan.ret);
    return tmptxclscan.ret;
}
