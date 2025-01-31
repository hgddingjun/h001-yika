/*
 * d.j create 2014.11.26 qn8006 module
 */
#ifndef ANDROID_QN8006_INTERFACE_H
#define ANDROID_QN8006_INTERFACE_H
 
#include <hardware/hardware.h>
 
__BEGIN_DECLS
 
/* define module interface */
#define QN8006_HARDWARE_MODULE_ID "qn8006"
 
/* from qn8006_driver.c */
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
 
 
/* define HW module structure */
struct qn8006_module_t {
    struct hw_module_t common;
};
 
/* define HW interface structure */
struct qn8006_device_t {
    struct hw_device_t common;
    int fd;
    int (*set_frequency)(struct qn8006_device_t *dev, int  fq);
    int (*get_frequency)(struct qn8006_device_t *dev, int *fq);
 
    int (*set_power)(struct qn8006_device_t *dev, int  pwer);
    int (*get_power)(struct qn8006_device_t *dev, int *pwer);
 
    int (*audio_mode)(struct qn8006_device_t *dev, int audmode);
    int (*audio_config)(struct qn8006_device_t *dev, int optiontype, int option, int isRx);
    int (*system_mode)(struct qn8006_device_t *dev, int sysmode);
    int (*init)(struct qn8006_device_t *dev);
 
    /* d.j add 2014.12.09 */
    int (*csPin)(struct qn8006_device_t *dev, int level);
    int (*getRSSI)(struct qn8006_device_t *dev, int ch);
    int (*rxSeekCHAll)(struct qn8006_device_t *dev, int start, int stop, int step, int db, int up);
    int (*rxSeekCH)(struct qn8006_device_t *dev, int start, int stop, int step, int db, int up);
    int (*RDSEnable)(struct qn8006_device_t *dev, int en);
    int (*RDSDetectSignal)(struct qn8006_device_t *dev ,int *sig);
    int (*RDSCheckBufferReady)(struct qn8006_device_t *dev, int *chkbufready);
    int (*RDSLoadData)(struct qn8006_device_t *dev, int *rawData, int len, int isUpload);
    int (*setCountry)(struct qn8006_device_t *dev, int country);
    int (*configFMModule)(struct qn8006_device_t *dev, int optiontype, int option);
    int (*loadDefalutSetting)(struct qn8006_device_t *dev, int country);
    int (*txClearChannelScan)(struct qn8006_device_t *dev, int start, int stop, int step, int db);
    int (*powerEnable)(struct qn8006_device_t *dev, int en);
};
 
 
__END_DECLS
 
#endif /* ANDROID_QN8006_INTERFACE_H */