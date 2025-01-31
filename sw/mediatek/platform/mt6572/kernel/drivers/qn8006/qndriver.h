/************************************************Copyright(c)***********************************
**                                   Quintic(Nanjing) Microelectronics Co,Ltd.
**                                   
**                                   http://www.quinticcorp.com
**
**--------------------File Info----------------------------------------------------------------
** File Name:                  qndriver.h
** subversion number:   160      
**----------------------------------------------------------------------------------------
************************************************************************************************/

#ifndef _QNDRIVER_H
#define _QNDRIVER_H
#define QN_8006B1
#define _QNFUNC_H_


#define CCS_RX  0
#define CCS_TX  1

#define FREQ2CHREG(freq)   ((freq-7600)/5)
#define CHREG2FREQ(ch)     (ch*5+7600)


#define _QNCOMMON_H_




#define QND_REG_NUM_MAX   94  // for qn8006b1
// crystal setting definition is not ready yet, please read datasheet to do setting accordingly
#define QND_CRYSTAL_REG             0x03 //?
#define QND_CRYSTAL_BIT_MASK        0x0f //?

#define QND_CRYSTAL_26MHZ           0x2b //?
#define QND_CRYSTAL_DEFAULT         QND_CRYSTAL_26MHZ


#define S_XDATA 
#define S_IDATA
#define S_BIT unsigned char
#define S_SBIT unsigned char
#define S_CODE
#define S_SFR unsigned char

#define QND_MODE_SLEEP      0     
#define QND_MODE_WAKEUP     1
#define QND_MODE_DEFAULT    2
// RX / TX value is using upper 8 bit

#define QND_MODE_RX         0x8000
#define QND_MODE_TX         0x4000
// AM / FM value is using lower 8 bit 
// need to check datasheet to get right bit 
#define QND_MODE_FM         0x0000

#define BAND_FM        0


// tune
#define QND_FSTEP_50KHZ      0
#define QND_FSTEP_100KHZ      1
#define QND_FSTEP_200KHZ      2
// output format
#define QND_OUTPUT_ANALOG     0
#define QND_OUTPUT_IIS        1

// stereo mode
#define QND_TX_AUDIO_MONO              0x10
#define QND_TX_AUDIO_STEREO            0x00

#define QND_RX_AUDIO_MONO              0x20
#define QND_RX_AUDIO_STEREO            0x00

#define QND_CONFIG_MONO               0x01
#define QND_CONFIG_MUTE               0x02    
#define QND_CONFIG_SOFTCLIP           0x03
#define QND_CONFIG_AUTOAGC               0x04
#define QND_CONFIG_AGCGAIN               0x05    

#define QND_CONFIG_EQUALIZER           0x06    
#define QND_CONFIG_VOLUME               0x07          
#define QND_CONFIG_BASS_QUALITY       0x08
#define QND_CONFIG_BASS_FREQ           0x09
#define QND_CONFIG_BASS_GAIN           0x0a
#define QND_CONFIG_MID_QUALITY        0x0b
#define QND_CONFIG_MID_FREQ           0x0c
#define QND_CONFIG_MID_GAIN           0x0d
#define QND_CONFIG_TREBLE_FREQ        0x0e
#define QND_CONFIG_TREBLE_GAIN        0x0f

#define QND_ENABLE_EQUALIZER          0x10
#define QND_DISABLE_EQUALIZER         0x00


#define QND_CONFIG_AUDIOPEAK_DEV      0x11
#define QND_CONFIG_PILOT_DEV          0x12
#define QND_CONFIG_RDS_DEV            0x13

// input format
#define QND_INPUT_ANALOG     0
#define QND_INPUT_IIS        1

// i2s mode
#define QND_I2S_RX_ANALOG   0x00
#define QND_I2S_RX_DIGITAL  0x40
#define QND_I2S_TX_ANALOG   0x00
#define QND_I2S_TX_DIGITAL  0x20

//i2s clock data rate
#define QND_I2S_DATA_RATE_32K  0x00
#define QND_I2S_DATA_RATE_40K  0x10
#define QND_I2S_DATA_RATE_44K  0x20
#define QND_I2S_DATA_RATE_48K  0x30

//i2s clock Bit Wise
#define QND_I2S_BIT_8    0x00
#define QND_I2S_BIT_16   0x40
#define QND_I2S_BIT_24   0x80
#define QND_I2S_BIT_32   0xc0

//i2s Control mode
#define QND_I2S_MASTER   1
#define QND_I2S_SLAVE    0

//i2s Control mode
#define QND_I2S_MSB   0x00
#define QND_I2S_I2S   0x01
#define QND_I2S_DSP1  0x02
#define QND_I2S_DSP2  0x03
#define QND_I2S_LSB   0x04

#define QND_EQUALIZE_BASS    0x00
#define QND_EQUALIZE_MID    0x01
#define QND_EQUALIZE_TREBLE 0x02
// RDS, TMC
#define QND_EUROPE_FLEXIBILITY_DISABLE  0
#define QND_EUROPE_FLEXIBILITY_ENABLE   1
#define QND_RDS_OFF              0
#define QND_RDS_ON               1
#define QND_RDS_BUFFER_NOT_READY 0
#define QND_RDS_BUFFER_READY     1


#define CHIPID_QN8000    0x00
#define CHIPID_QN8005    0x20
#define CHIPID_QN8005B1  0x21
#define CHIPID_QN8006    0x30
#define CHIPID_QN8006LB  0x71
#define CHIPID_QN8007B1  0x11
#define CHIPID_QN8007    0x10
#define CHIPID_QN8006A1  0x30
#define CHIPID_QN8006B1  0x31
#define CHIPID_QN8016    0xe0
#define CHIPID_QN8016_1  0xb0
#define CHIPID_QN8015    0xa0
#define CHIPID_QN8065    0xa0
#define CHIPID_QN8067    0xd0
#define CHIPID_QN8065N   0xa0
#define CHIPID_QN8027    0x40
#define CHIPID_QN8025    0x80

#define RDS_INT_ENABLE  1
#define RDS_INT_DISABLE 0
//For antena impedance match
#define QND_HIGH_IMPEDANCE         1
#define QND_LOW_IMPEDANCE         0

#define QND_BAND_NUM     6
#define RSSINTHRESHOLD   4

#ifndef UINT8
typedef unsigned char  UINT8;
#endif


#ifndef UINT16
typedef unsigned short UINT16;
#endif

#define FALSE 0
#define TRUE 1
//typedef short          INT16;    
/*typedef unsigned int   UINT32;            
typedef signed   int   INT32;  
typedef float          FP32;              
typedef double         FP64;              
*/



#define _QNCONFIG_H_



/********************* country selection**************/
#define COUNTRY_CHINA            0
#define COUNTRY_USA                1
#define COUNTRY_JAPAN            2
/************************EDN******************************/

/********************* minor feature selection*********************/

#define  QN_CCA_MAX_CH     50

/**********************************************************************************************
// Performance configuration 
***********************************************************************************************/
#define SMSTART_VAL     8
#define HCCSTART_VAL    22
#define SNCSTART_VAL    40


/**********************************************************************************************
// limitation configuration 
***********************************************************************************************/

#define QND_READ_RSSI_DELAY    10
#define QND_DELAY_BEFORE_UNMUTE  200
// auto scan
#define QND_MP_THRESHOLD       0x28   
#define PILOT_READ_OUT_DELAY_TIME 70
#define PILOT_SNR_READ_OUT_DELAY_TIME  (150-PILOT_READ_OUT_DELAY_TIME)
#define CH_SETUP_DELAY_TIME    300           


#define assert(str)
#define QND_LOG(a)
#define QND_LOGA(a,b)
#define QND_LOGB(a,b)
#define QND_LOGHEX(a,b)
#define _QNREG_H_
#define SYSTEM1    0x00
#define SYSTEM2    0x01
#define DEV_ADD    0x02
#define ANACTL1    0x03
#define REG_VGA    0x04
#define CID1       0x05
#define CID2       0x06
#define I2S        0x07
#define CH         0x08
#define CH_START   0x09
#define CH_STOP    0x0A
#define CH_STEP    0x0B
#define PAC_TARGET 0x0C
#define TXAGC_GAIN 0x0D
#define TX_FDEV    0x0E
#define GAIN_TXPLT 0x0F
#define RDSD0      0x10
#define RDSD1      0x11
#define RDSD2      0x12
#define RDSD3      0x13
#define RDSD4      0x14
#define RDSD5      0x15
#define RDSD6      0x16
#define RDSD7      0x17
#define RDSFDEV    0x18
#define CCA        0x19
#define STATUS1    0x1A
#define STATUS3    0x1B
#define RSSISIG    0x1C

// for engineering usage
// SYSTEM1   define 16bits macros for AM, RX|FM, TX|FM, RX|AM
#define RXREQ      0x80
#define TXREQ      0x40
#define CHSC       0x20
#define STNBY      0x10
#define RXI2S      0x08
#define TXI2S      0x04
#define RDSEN      0x02
#define CCA_CH_DIS 0x01

// SYSTEM2
#define SWRST      0x80
#define RECAL      0x40
#define FORCE_MO   0x20
#define ST_MO_TX   0x10
#define PETC       0x08
#define RDSTXRDY   0x04
#define TMOUT1     0x02
#define TMOUT0     0x01
// CH         0x08
#define FSTEP         0xc0
#define CH_CH_STOP    0x30
#define CH_CH_START   0x0c
#define CH_CH         0x03

// CCA
#define TXCCAA      0xe0
#define RXCCAA      0x1f

// RDSFDEV
#define RDS_ONLY    0x80
#define  RDSFDEV_   0x3f
// TXAGC_GAIN 0x0D
#define TX_SFTCLPEN         0x80
#define TAGC_GAIN_SEL       0x40
#define IMR                 0x20
#define TXAGC_GDB           0x10
#define TXAGC_GVGA3         0x08
#define TXAGC_GVGA2         0x04
#define TXAGC_GVGA1         0x02
#define TXAGC_GVGA0         0x01
#define TXAGC_GAINALL       0x0f
// STATUS1
#define I2SOVFL       0x20
#define I2SUNDFL      0x10
#define INSAT         0x08
#define RXAGCSET      0x04
#define RXAGCERR      0x02
#define ST_MO_RX      0x01
// STATUS3
#define RDS_RXUPD     0x80
#define E_DET         0x40
#define RDSC0C1       0x20
#define RDSSYNC       0x10
#define RDS0ERR       0x08
#define RDS1ERR       0x04
#define RDS2ERR       0x02
#define RDS3ERR       0x01
#define RDSERR        0x0f
// PAG_CAL
#define PAG_HD      0x80
#define PAG_DIS     0x40
#define IPOW        0xc0
#define PAGAIN      0x3f

#define MUTE         0x0c
#define UNMUTE       0x00
#define RX_MONO_MASK 0x20
#define RSSIMP        0x21
#define PAG_CAL       0x5a
#define GAIN_SEL      0x1f
#define HYSTERSIS     0x3f
#define SMSTART       0x40
#define SNCSTART      0x41
#define HCCSTART      0x42
#define MPSTART       0x43
#define REG_LNA       0x4d
#define REG_PD2       0x52
#define SNR           0x22
#define CCOND1        0x38
#define CCA2          0x35
#define CCA4          0x36
#define CCA5          0x37
#define TXAGC5        0x2e
#define _QNSYS_H_

#define CHANNEL_FILTER

// external driver interface 
// logical layer
/*****************************************************************************
Driver API Macro Definition
*****************************************************************************/
#define QNM_SetCrystal(Crystal)    \
        QNF_SetRegBit(QND_CRYSTAL_REG, QND_CRYSTAL_BIT_MASK, Crystal)     
#define QNM_SetAudioInputImpedance(AudioImpendance) \
        qn8006_gpio_i2c_WriteReg(REG_VGA, qn8006_gpio_i2c_ReadReg(REG_VGA) | (AudioImpendance & 0x3f))   
#define QNM_ResetToDefault() \
        qn8006_gpio_i2c_WriteReg(SYSTEM2, SWRST) 
#define QNM_SetFMWorkingMode(Modemask, Mode) \
        qn8006_gpio_i2c_WriteReg(SYSTEM1, Mode|(qn8006_gpio_i2c_ReadReg(SYSTEM1) &~ Modemask)
#define QNM_EnableAGC() \
        qn8006_gpio_i2c_WriteReg(TXAGC_GAIN, ~TAGC_GAIN_SEL&(qn8006_gpio_i2c_ReadReg(TXAGC_GAIN)))
#define QNM_DisableAGC()\
        qn8006_gpio_i2c_WriteReg(TXAGC_GAIN,   1|(TAGC_GAIN_SEL|(qn8006_gpio_i2c_ReadReg(TXAGC_GAIN)) )
#define QNM_EnableSoftClip() \
        qn8006_gpio_i2c_WriteReg(TXAGC_GAIN,    TX_SFTCLPEN |(qn8006_gpio_i2c_ReadReg(TXAGC_GAIN)) )
#define QNM_DisableSoftClip() \
        qn8006_gpio_i2c_WriteReg(TXAGC_GAIN,    ~TX_SFTCLPEN &(qn8006_gpio_i2c_ReadReg(TXAGC_GAIN)) )
#define QNM_GetMonoMode() \
        qn8006_gpio_i2c_ReadReg(STATUS1) & ST_MO_RX
#define QNM_GetAGCStatus() \
        qn8006_gpio_i2c_ReadReg(TXAGC_GAIN) & TAGC_GAIN_SEL
#define QNM_SetRxThreshold(db) \
        qn8006_gpio_i2c_WriteReg(CCA, db)
#define QNM_SetAudioOutFormatIIS() \
        qn8006_gpio_i2c_WriteReg(CCA, (qn8006_gpio_i2c_ReadReg(CCA) | RXI2S))
#define QNM_SetAudioOutFormatAnalog() \
        qn8006_gpio_i2c_WriteReg(CCA, (qn8006_gpio_i2c_ReadReg(CCA) & ~RXI2S))
#define QNM_SetAudioInFormatIIS() \
        qn8006_gpio_i2c_WriteReg(CCA, (qn8006_gpio_i2c_ReadReg(CCA) | RXI2S))
#define QNM_SetAudioInFormatAnalog() \
        qn8006_gpio_i2c_WriteReg(CCA, (qn8006_gpio_i2c_ReadReg(CCA) & ~RXI2S))
#define QNM_GetRssi() \
        qn8006_gpio_i2c_ReadReg(RSSISIG)
#define QND_AntenaInputImpedance(impendance) \
        qn8006_gpio_i2c_WriteReg(77, impendance)

#define QND_READ(adr)    qn8006_gpio_i2c_ReadReg(adr)
#define QND_WRITE(adr, value)  qn8006_gpio_i2c_WriteReg(adr, value)
extern void   QNF_SetRegBit(UINT8 reg, UINT8 bitMask, UINT8 data_val) ;

extern UINT8  S_XDATA  qnd_RSSIns;
extern UINT8  S_XDATA  qnd_RSSIn[QND_BAND_NUM];
extern UINT16 S_XDATA  qnd_RSSInBB[QND_BAND_NUM+1];
extern UINT8  S_XDATA  qnd_Country;
extern UINT16 S_XDATA  qnd_CH_START;
extern UINT16 S_XDATA  qnd_CH_STOP;
extern UINT8  S_XDATA  qnd_CH_STEP;

/*
  System General Control 
*/
extern UINT16 QNF_GetCh(void);

extern void QND_Delay(UINT16 ms) ;
extern UINT8 QND_GetRSSI(UINT16 ch) ;
extern UINT8 QND_Init(void) ;
extern void  QND_TuneToCH(UINT16 ch) ;
extern void  QND_SetSysMode(UINT16 mode) ;
extern void  QND_SetCountry(UINT8 country) ;
extern void QND_ConfigFMModule(UINT8 optiontype, UINT8 option) ;

extern void QND_UpdateRSSIn(UINT16 ch) ;


#define QN_TX
#define _QNTX_H_
extern void   QND_TXSetPower(UINT8 gain);
extern void   QND_TXConfigAudio(UINT8 optiontype, UINT8 option );

#define QN_RX
#define _QNRX_H_
typedef void  (*QND_SeekCallBack)(UINT16 ch, UINT8 bandtype);
extern UINT8  S_XDATA qnd_ChCount;
extern UINT16 S_XDATA qnd_ChList[QN_CCA_MAX_CH];
extern UINT8  S_XDATA qnd_StepTbl[3];
extern UINT8  S_XDATA qnd_AutoScanAll;

extern void   QND_SetSeekCallBack(QND_SeekCallBack func);
extern void   QND_RXConfigAudio(UINT8 optiontype, UINT8 option) ;
extern UINT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up) ;
extern UINT8  QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up);
extern UINT16 QND_TXClearChannelScan(UINT16 start, UINT16 stop, UINT8 step,UINT8 db);
// patch 
#define QN_RDS

#define _QNRDS_H_

extern UINT8 QND_RDSEnable(UINT8 on) ;
extern UINT8 QND_RDSCheckBufferReady(void) ;
extern void QND_LoadDefalutSetting(UINT8 country);



extern UINT8 QND_RDSDetectSignal(void) ;
extern void  QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload) ;
#endif
