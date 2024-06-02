#include "qndriver.h"

/*d.j modify 2014.11.20*/
extern UINT8 qn8006_gpio_i2c_ReadReg(UINT8 adr);
extern UINT8 qn8006_gpio_i2c_WriteReg(UINT8 adr, UINT8 value);
void QND_Delay(unsigned short ms);


#define R_TXRX_MASK    0xd0
UINT8  S_XDATA  qnd_RSSIns = 255;
UINT8  S_XDATA  qnd_Crystal = QND_CRYSTAL_DEFAULT;
UINT8  S_XDATA  qnd_RSSIn[QND_BAND_NUM];
UINT8  S_XDATA  qnd_PrevMode;
UINT8  S_XDATA  qnd_Country  = COUNTRY_CHINA ;
UINT16 S_XDATA  qnd_CH_START = 7600;
UINT16 S_XDATA  qnd_CH_STOP  =  10800;
UINT8  S_XDATA  qnd_CH_STEP  = 1;
UINT8  qnd_ClearScanFlag = 0;
UINT16 qnd_ClearChannel = 0;
UINT8  qnd_FirstScan = 1;

UINT8  S_XDATA qnd_AutoScanAll;
UINT8  S_XDATA qnd_IsStereo;
UINT8  S_XDATA qnd_ChCount;
UINT16 S_XDATA  qnd_RSSInBB[QND_BAND_NUM+1];          
UINT16 S_XDATA qnd_ChList[QN_CCA_MAX_CH];
UINT8  S_XDATA  qnd_StepTbl[3]={5,10,20};
QND_SeekCallBack qnd_CallBackFunc = 0;


/**********************************************************************
void QNF_RXInit()
**********************************************************************
Description: set to SNR based MPX control. Call this function before 
             tune to one specific channel

Parameters:
None
Return Value:
None
**********************************************************************/
void QNF_RXInit()
{
    QNF_SetRegBit(77,0x80,0x80);  // Set to High-Z
    QNF_SetRegBit(40,0x80,0x80);  // Set SNR as criteria for MPX control

    QNF_SetRegBit(64,0x3f,SMSTART_VAL); //set SMSTART
    QNF_SetRegBit(66,0x3f,HCCSTART_VAL); //set HCCSTART
    QNF_SetRegBit(65,0x7f,SNCSTART_VAL); //set SNCSTART
}

/**********************************************************************
void QNF_SetMute(UINT8 On)
**********************************************************************
Description: set register specified bit

Parameters:
On:        1: mute, 0: unmute
Return Value:
None
**********************************************************************/
void QNF_SetMute(UINT8 On)
{
    if(On)
    {
        QNF_SetRegBit(REG_PD2, 0x0c, 0x0c); // mute/unmute to avoid noise    
        //   QNF_SetRegBit(3, 0x80, 0x80); // mute/unmute to avoid noise    
    }
    else
    {
        QND_Delay(QND_DELAY_BEFORE_UNMUTE);
        QNF_SetRegBit(REG_PD2, 0x0c, 0x00); // mute/unmute to avoid noise    
        //   QND_Delay(50);
     //   QNF_SetRegBit(3, 0x80, 0x00); // mute/unmute to avoid noise    
    }
}

/**********************************************************************
void QNF_SetRegBit(UINT8 reg, UINT8 bitMask, UINT8 data_val)
**********************************************************************
Description: set register specified bit

Parameters:
    reg:        register that will be set
    bitMask:    mask specified bit of register
    data_val:    data will be set for specified bit
Return Value:
    None
**********************************************************************/
void QNF_SetRegBit(UINT8 reg, UINT8 bitMask, UINT8 data_val) 
{
    UINT8 temp;
    temp = qn8006_gpio_i2c_ReadReg(reg);
    temp &= (UINT8)(~bitMask);
    temp |= data_val & bitMask;
//    temp |= data_val;
    qn8006_gpio_i2c_WriteReg(reg, temp);
}

/**********************************************************************
void QNF_InitRSSInBB()
**********************************************************************
Description: init RSSI noise floor band

Parameters:
    None
Return Value:
    None
**********************************************************************/
void QNF_InitRSSInBB() 
{
    UINT8 i,d2,d,step; 
    UINT16 d1;

    // get frequency leap step
    step = qnd_StepTbl[qnd_CH_STEP];
    // total frequency span
    d1 = qnd_CH_STOP - qnd_CH_START;
    // make sure d2 value <= 255, normally QND_BAND_NUM should < 10
    d2 = step * QND_BAND_NUM;
    d = d1/d2;
    // first one should be CH_START
    qnd_RSSInBB[0] = qnd_CH_START;
    for(i=1; i<QND_BAND_NUM; i++) {
        qnd_RSSInBB[i] = qnd_RSSInBB[i-1] + d * step;
    }
    // last one set one more step higher for convenience
    qnd_RSSInBB[i] = qnd_CH_STOP+step;
}

/**********************************************************************
UINT8 QNF_GetRSSInBandIndex(UINT16 chfreq) 
**********************************************************************
Description: get band index

Parameters:
    chfreq: channel frequency (10Khz unit, eg: for 101.10Mhz, input 10110)
Return Value: band index for input CH frequency
**********************************************************************/
UINT8 QNF_GetRSSInBandIndex(UINT16 chfreq)  
{
    UINT8 i;
    for(i=0; i<QND_BAND_NUM; i++) {
        if(chfreq < qnd_RSSInBB[i+1]) {
            // finally come to here, if no wrong input
            return i;
        }
    }
		return 0;
}

/**********************************************************************
UINT8 QNF_GetRSSIn(UINT16 chFreq)
**********************************************************************
Description: get RSSI noise floor

Parameters:
    chfreq: channel frequency (10Khz unit, eg: for 101.10Mhz, input 10110)
Return Value: 
    the RSSI noise floor
**********************************************************************/
UINT8 QNF_GetRSSIn(UINT16 chFreq) 
{
    UINT8 idx;
    idx = QNF_GetRSSInBandIndex(chFreq);
    return qnd_RSSIn[idx];
}

/**********************************************************************
UINT16 QNF_GetCh()
**********************************************************************
Description: get current channel frequency

Parameters:
    None
Return Value:
    channel frequency
**********************************************************************/
UINT16 QNF_GetCh(void) 
{
    UINT8 tCh;
    UINT8  tStep; 
    UINT16 ch = 0;
    // set to reg: CH_STEP
    tStep = qn8006_gpio_i2c_ReadReg(CH_STEP);
    tStep &= CH_CH;
    ch  =  tStep ;
    tCh= qn8006_gpio_i2c_ReadReg(CH);    
    ch = (ch<<8)+tCh;
    return CHREG2FREQ(ch);
}

/**********************************************************************
UINT8 QNF_SetCh(UINT16 freq)
**********************************************************************
Description: set channel frequency 

Parameters:
    freq:  channel frequency to be set
Return Value:
    1: success
**********************************************************************/
UINT8 QNF_SetCh(UINT16 freq) 
{
    // calculate ch parameter used for register setting
    UINT8 tStep;
    UINT8 tCh;
    UINT16 f; 
        f = FREQ2CHREG(freq); 
        // set to reg: CH
        tCh = (UINT8) f;
        qn8006_gpio_i2c_WriteReg(CH, tCh);
        // set to reg: CH_STEP
        tStep = qn8006_gpio_i2c_ReadReg(CH_STEP);
        tStep &= ~CH_CH;
        tStep |= ((UINT8) (f >> 8) & CH_CH);
        qn8006_gpio_i2c_WriteReg(CH_STEP, tStep);

    return 1;
}

/**********************************************************************
void QNF_ConfigScan(UINT16 start,UINT16 stop, UINT8 step)
**********************************************************************
Description: config start, stop, step register for FM/AM CCA or CCS

Parameters:
    start
        Set the frequency (10kHz) where scan to be started,
        eg: 7600 for 76.00MHz.
    stop
        Set the frequency (10kHz) where scan to be stopped,
        eg: 10800 for 108.00MHz
    step        
        1: set leap step to (FM)100kHz / 10kHz(AM)
        2: set leap step to (FM)200kHz / 1kHz(AM)
        0:  set leap step to (FM)50kHz / 9kHz(AM)
Return Value:
         None
**********************************************************************/
void QNF_ConfigScan(UINT16 start,UINT16 stop, UINT8 step) 
{
    // calculate ch para
    UINT8 tStep = 0;
    UINT8 tS;
    UINT16 fStart;
    UINT16 fStop;
        fStart = FREQ2CHREG(start);
        fStop = FREQ2CHREG(stop);
        // set to reg: CH_START
    tS = (UINT8) fStart;
    qn8006_gpio_i2c_WriteReg(CH_START, tS);
    tStep |= ((UINT8) (fStart >> 6) & CH_CH_START);
    // set to reg: CH_STOP
    tS = (UINT8) fStop;
    qn8006_gpio_i2c_WriteReg(CH_STOP, tS);
    tStep |= ((UINT8) (fStop >> 4) & CH_CH_STOP);
    // set to reg: CH_STEP
    tStep |= step << 6;
    qn8006_gpio_i2c_WriteReg(CH_STEP, tStep);
}

/**********************************************************************
void QNF_SetAudioMono(UINT8 modemask, UINT8 mode) 
**********************************************************************
Description:    Set audio output to mono.

Parameters:
  modemask: mask register specified bit
  mode
        QND_RX_AUDIO_MONO:    RX audio to mono
        QND_RX_AUDIO_STEREO:  RX audio to stereo    
        QND_TX_AUDIO_MONO:    TX audio to mono
        QND_TX_AUDIO_STEREO:  TX audio to stereo 
Return Value:
  None
**********************************************************************/
void QNF_SetAudioMono(UINT8 modemask, UINT8 mode) 
{
    if (mode == QND_TX_AUDIO_MONO||mode == QND_RX_AUDIO_MONO) 
    {
        // set to 22.5K (22.5/0.69 ~= 32)
        qn8006_gpio_i2c_WriteReg(TX_FDEV, 0x20);
    } 
    else 
    {
        // back to default
        qn8006_gpio_i2c_WriteReg(TX_FDEV, 0x6c);
    }
    QNF_SetRegBit(SYSTEM2,modemask, mode);
}

/**********************************************************************
void QNF_UpdateRSSIn(UINT16 chFreq)
**********************************************************************
Description: update the qnd_RSSIns and qnd_RSSIn value              
Parameters:
    None
Return Value:
    None
**********************************************************************/
void  QNF_UpdateRSSIn(UINT16 chFreq) 
{
    UINT8 i;
    UINT8 r0;
    UINT16 ch;

    // backup SYSTEM1 register
    r0 = qn8006_gpio_i2c_ReadReg(SYSTEM1);
    if(!chFreq) 
    {
        for (i = 0 ; i < QND_BAND_NUM; i++)
        {
            ch = QND_TXClearChannelScan(qnd_RSSInBB[i], qnd_RSSInBB[i+1]-qnd_StepTbl[qnd_CH_STEP], qnd_CH_STEP, 15);
            qnd_RSSIn[i] = QND_GetRSSI(ch);
            if (qnd_RSSIns > qnd_RSSIn[i])
            {
                qnd_RSSIns = qnd_RSSIn[i];
                qnd_ClearChannel = ch;
            }
        }
    } 
    else
    {
        i = QNF_GetRSSInBandIndex(chFreq);
        ch = QND_TXClearChannelScan(qnd_RSSInBB[i], qnd_RSSInBB[i+1]-qnd_StepTbl[qnd_CH_STEP], qnd_CH_STEP, 15);
        qnd_RSSIn[i] = QND_GetRSSI(ch);
    }
    // restore SYSTEM1 register
    qn8006_gpio_i2c_WriteReg(SYSTEM1, r0);
}

#if 0
/**********************************************************************
int QND_Delay()
**********************************************************************
Description: Delay for some ms, to be customized according to user
             application platform

Parameters:
        ms: ms counts
Return Value:
        None
            
**********************************************************************/
void QND_Delay(UINT16 ms) 
{
    UINT16 i,k;
    for(i=0; i<3000;i++) 
    {    
        for(k=0; k<ms; k++) 
        {

        }
    }
}
#endif



/**********************************************************************
UINT8 QND_GetRSSI(UINT16 ch)
**********************************************************************
Description:    Get the RSSI value
Parameters:
Return Value:
RSSI value  of the channel setted
**********************************************************************/
UINT8 QND_GetRSSI(UINT16 ch) 
{
    UINT8 delayTime;
    {
        QND_SetSysMode(QND_MODE_RX|QND_MODE_FM); 
        delayTime = QND_READ_RSSI_DELAY;
    }
    QNF_SetCh(ch);
    QNF_SetRegBit(SYSTEM1, CCA_CH_DIS, CCA_CH_DIS);    
    QND_Delay(delayTime); 
    return qn8006_gpio_i2c_ReadReg(RSSISIG);  
}

/**********************************************************************
void QN_ChipInitialization()
**********************************************************************
Description: chip first step initialization, called only by QND_Init()

Parameters:
    None
Return Value:
    None
**********************************************************************/
void QN_ChipInitialization()
{
    qn8006_gpio_i2c_WriteReg(0x01,0x89);  //sw reset
    qn8006_gpio_i2c_WriteReg(0x50,0x00);
    qn8006_gpio_i2c_WriteReg(0x01,0x49);  //recalibrate
    qn8006_gpio_i2c_WriteReg(0x01,0x09);
    QND_Delay(410);           //wait more than 400ms
    qn8006_gpio_i2c_WriteReg(0x3C,0x89);  //for RDS SYNC
    QNF_SetRegBit(0x49,0xc0, 0x00);
    qn8006_gpio_i2c_WriteReg(0x4a,0xba);
    qn8006_gpio_i2c_WriteReg(0x5c,0x05);
    qn8006_gpio_i2c_WriteReg(0x52,0x0c);
    QNF_SetRegBit(49,0x30,0x00); //minimize rxagc_timeout; WQF0717
    qn8006_gpio_i2c_WriteReg(0x38,0x2d);  //for SNR CCA ccnclk2=2'b00
    qn8006_gpio_i2c_WriteReg(0x39,0x88);  //for SNR CCA ccnclk1=2'b00
    qn8006_gpio_i2c_WriteReg(0x52,0x0c);  //mute
    qn8006_gpio_i2c_WriteReg(0x00,0x81);
    qn8006_gpio_i2c_WriteReg(0x57,0x80);
    qn8006_gpio_i2c_WriteReg(0x57,0x00);
    QND_Delay(120);           //wait more than 100ms
    qn8006_gpio_i2c_WriteReg(0x00,0x01);
    qn8006_gpio_i2c_WriteReg(0x52,0x00);

    QNF_SetRegBit(31,0x38,0x28);  //WQF0717
    //WQF0717  
    qn8006_gpio_i2c_WriteReg(HYSTERSIS, 0xff);
    qn8006_gpio_i2c_WriteReg(MPSTART, 0x12);
    QNF_SetRegBit(58,0xf,0xf);//set ccth11
		
}

/**********************************************************************
void QND_LoadDefalutSetting(UINT8 country)
**********************************************************************
Description: load some defalut setting for a certain country
Parameters:
    country :
    COUNTRY_CHINA: China
    COUNTRY_USA: USA
    COUNTRY_JAPAN: Japan (not supported yet)
Return Value:
    None
**********************************************************************/
void QND_LoadDefalutSetting(UINT8 country)
{

    switch(country)
    {
    case COUNTRY_CHINA:
        break;
    case COUNTRY_USA:
        QND_ConfigFMModule(QND_CONFIG_AUDIOPEAK_DEV,75);
        QND_ConfigFMModule(QND_CONFIG_PILOT_DEV,9);
        QND_ConfigFMModule(QND_CONFIG_RDS_DEV,2);
        QNF_SetRegBit(0x01,0x08,0x08);
        QNF_SetRegBit(0x04,0x3f,0x3f);
        QND_TXConfigAudio(QND_CONFIG_SOFTCLIP,1);
        break;
    case COUNTRY_JAPAN:
        break;
    default:
        break;
    }
}

/**********************************************************************
int QND_Init()
**********************************************************************
Description: Initialize device to make it ready to have all functionality ready for use.

Parameters:
    None
Return Value:
    1: Device is ready to use.
    0: Device is not ready to serve function.
**********************************************************************/
UINT8 QND_Init(void) 
{
    QN_ChipInitialization();
    QNF_SetMute(1);
    QNF_InitRSSInBB();    // init band range
    qn8006_gpio_i2c_WriteReg(0x00,  0x01); //resume original status of chip /* 2008 06 13 */
    QNF_SetMute(0);
	
    return 1;
}

/**********************************************************************
void QND_SetSysMode(UINT16 mode)
***********************************************************************
Description: Set device system mode(like: sleep ,wakeup etc) 
Parameters:
    mode:  set the system mode , it will be set by  some macro define usually:
    
    SLEEP (added prefix: QND_MODE_, same as below):  set chip to sleep mode
    WAKEUP: wake up chip 
    TX:     set chip work on TX mode
    RX:     set chip work on RX mode
    FM:     set chip work on FM mode
    AM:     set chip work on AM mode
    TX|FM:  set chip work on FM,TX mode
    RX|AM;  set chip work on AM,RX mode
    RX|FM:    set chip work on FM,RX mode
Return Value:
    None     
**********************************************************************/
void QND_SetSysMode(UINT16 mode) 
{    
    UINT8 val;
    switch(mode)        
    {        
    case QND_MODE_SLEEP:                       //set sleep mode        
        qnd_PrevMode = qn8006_gpio_i2c_ReadReg(SYSTEM1);        
        QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, STNBY);         
        break;        
    case QND_MODE_WAKEUP:                      //set wakeup mode        
        qn8006_gpio_i2c_WriteReg(SYSTEM1, qnd_PrevMode);        
        break;        
    case QND_MODE_DEFAULT:
        QNF_SetRegBit(SYSTEM2,0x80,0x80);
        break;
    default:    
            val = (UINT8)(mode >> 8);        
            if (val)
            {
                    if(val == (QND_MODE_TX >> 8))
                    {
                        qn8006_gpio_i2c_WriteReg(73, 0x44);
                    }
                    else if(val == (QND_MODE_RX >> 8))
                    {
                        qn8006_gpio_i2c_WriteReg(73, 0x04);
                    }                        
                    // set to new mode if it's not same as old
                    if((qn8006_gpio_i2c_ReadReg(SYSTEM1) & R_TXRX_MASK) != val)
                    {
                        QNF_SetMute(1);
                        QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, val); 
                        //   QNF_SetMute(0);
                    }
                    // make sure it's working on analog output
                    QNF_SetRegBit(SYSTEM1, 0x08, 0x00);    
            }    
        break;        
    }    
}

/**********************************************************************
void QND_TuneToCH(UINT16 ch)
**********************************************************************
Description: Tune to the specific channel. call QND_SetSysMode() before 
    call this function
Parameters:
    ch
    Set the frequency (10kHz) to be tuned,
    eg: 101.30MHz will be set to 10130.
Return Value:
    None
**********************************************************************/
void QND_TuneToCH(UINT16 ch) 
{
//    UINT8 rssi;
//    UINT8 minrssi;

    QNF_RXInit();

    // if chip working on TX mode, just set CH
    if(qn8006_gpio_i2c_ReadReg(SYSTEM1) & TXREQ) 
    {
        QNF_SetRegBit(SYSTEM1, CCA_CH_DIS, CCA_CH_DIS);
        QNF_SetCh(ch);   
        return;
    }
    QNF_SetMute(1);
    if ((ch - 7710) % 240 == 0) 
    {
        QNF_SetRegBit(TXAGC_GAIN, IMR, IMR);
    } 
    else 
    {
        QNF_SetRegBit(TXAGC_GAIN, IMR, 0x00);
    }
    QNF_SetCh(ch);
    QNF_SetRegBit(SYSTEM1, CCA_CH_DIS, CCA_CH_DIS);

    //Free filter without RDS and Set filter 2 for Non-RDS
    QNF_SetRegBit(GAIN_SEL,0x38,(qn8006_gpio_i2c_ReadReg(SYSTEM2) & 0x20) ? 0x00 : 0x28); 
    QNF_SetMute(0);
}

/**********************************************************************
void QND_SetCountry(UINT8 country)
***********************************************************************
Description: Set start, stop, step for RX and TX based on different
             country
Parameters:
country:
Set the chip used in specified country:
    CHINA:
    USA:
    JAPAN:
Return Value:
    None     
**********************************************************************/
void QND_SetCountry(UINT8 country) 
{
    qnd_Country = country;
    switch(country)
    {
    case COUNTRY_CHINA:
        qnd_CH_START = 7600;
        qnd_CH_STOP = 10800;
        qnd_CH_STEP = 1;
        break;
    case COUNTRY_USA:
        qnd_CH_START = 8810;
        qnd_CH_STOP = 10790;
        qnd_CH_STEP = 2;
        break;
    case COUNTRY_JAPAN:
        break;
    }
}

/**********************************************************************
void QND_UpdateRSSIn(UINT16 ch)
**********************************************************************
Description: in case of environment changed, we need to update RSSI noise floor
Parameters:
    None
Return Value:
    None
**********************************************************************/
void QND_UpdateRSSIn(UINT16 ch) 
{
    UINT8 temp;
    UINT8 v_abs;
    if (qnd_FirstScan == 0 )
    {
        temp = QND_GetRSSI(qnd_ClearChannel);

        if(temp > qnd_RSSIns)
        {
            v_abs = temp - qnd_RSSIns;
        }
        else
        {
            v_abs = qnd_RSSIns - temp;
        }
        if (v_abs< RSSINTHRESHOLD)
        {
            qnd_ClearScanFlag = 0;    
        }
        else
        {
            qnd_ClearScanFlag = 1;
        }
    }
    if (qnd_ClearScanFlag||qnd_FirstScan||ch)
    {
        QNF_UpdateRSSIn(ch);
        qnd_FirstScan = 0;
    }
    return;
}

/**********************************************************************
void QND_ConfigFMModule(UINT8 optiontype, UINT8 option)
***********************************************************************
Description: Config the FM modulation setting
country
Parameters:
  optiontype:
    QND_CONFIG_AUDIOPEAK_DEV : audio peak deviation
    QND_CONFIG_PILOT_DEV:      pilot deviation
    QND_CONFIG_RDS_DEV:        rds deviation  
  option:
    QND_CONFIG_AUDIOPEAK_DEV: 0~165khz
    QND_CONFIG_PILOT_DEV:     8~10
    QND_CONFIG_RDS_DEV:       1~7.5khz
Return Value:
    None     
**********************************************************************/
void QND_ConfigFMModule(UINT8 optiontype, UINT8 option) 
{
    UINT8  tmp8;
    UINT16 tmp16;
    switch (optiontype)
    {
    case QND_CONFIG_AUDIOPEAK_DEV:
        tmp16 = option*100;
        tmp8  = tmp16/69;
        qn8006_gpio_i2c_WriteReg(0x0e,tmp8);
        break;
    case QND_CONFIG_PILOT_DEV:
        tmp8 = option<<2;
        QNF_SetRegBit(0x0f,0x3c,tmp8);
        break;
    case QND_CONFIG_RDS_DEV:
        tmp16 = option*100;
        tmp8  = tmp16/35;
        QNF_SetRegBit(0x18,0x7f,tmp8);
        break;
    default:
        break;
    }
}

/***********************************************************************
Description: set call back function which can be called between seeking 
channel
Parameters:
func : the function will be called between seeking
Return Value:
None
**********************************************************************/
void QND_SetSeekCallBack(QND_SeekCallBack func)
{
    qnd_CallBackFunc = func;
}

/***********************************************************************
UINT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT16 step, UINT8 snr_th, UINT8 up);
***********************************************************************
Description: Automatically scans the frequency range, and detects the 
		first channel(AM or FM, it will be determine by the system mode which set 
		by QND_SetSysMode).
		A threshold value needs to be passed in for channel detection.
Parameters:
	start
		Set the frequency (10kHz) where scan will be started,
		eg: 76.00MHz will be set to 7600.
	stop
		Set the frequency (10kHz) where scan will be stopped,
		eg: 108.00MHz will be set to 10800.
	step
		FM:
			QND_FMSTEP_100KHZ: set leap step to 100kHz
			QND_FMSTEP_200KHZ: set leap step to 200kHz
			QND_FMSTEP_50KHZ:  set leap step to 50kHz
		AM:
			QND_AMSTEP_***:
	snr_th:
		Set threshold for quality of channel to be searched. 
	up:
		Set the seach direction :
			Up;0,seach from stop to start
			Up:1 seach from start to stop
Return Value:
  The channel frequency (unit: 10kHz)
  -1: no channel found

***********************************************************************/
UINT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up) 
{
    UINT8 regValue;
    UINT16 c, chfreq;
    UINT16 ifcnt;
//    UINT8 savevalue;
    UINT8 rssi;//,mprssi;
    UINT8 scanFlag = 1;
    UINT8 stepvalue;
    UINT8 timeout = 0;
    UINT8 pilot_snr;
    UINT8 minrssi;
    if (qnd_AutoScanAll == 0)
    {
        QNF_SetMute(1);
        QNF_SetRegBit(GAIN_SEL,0x38,0x38);
        QND_UpdateRSSIn(0);	
    }
    up=(start<stop) ? 1 : 0;
    stepvalue = qnd_StepTbl[step];
    do
    {
        minrssi = QNF_GetRSSIn(start);
        QNF_SetCh(start);
        c = start;
        if ((c - 7710) % 240 == 0) 
        { 		
            QNF_SetRegBit(TXAGC_GAIN, IMR, IMR);
        } 
        else 
        {
            QNF_SetRegBit(TXAGC_GAIN, IMR, 0);
        }	
        QNF_SetRegBit(SYSTEM1,0xA1,0x81);
        QND_Delay(10); //10ms delay
        rssi = qn8006_gpio_i2c_ReadReg(RSSISIG);	//read RSSI
        if (rssi > minrssi+6+db)
        {	
            QNF_ConfigScan(start, start, step);
            QNM_SetRxThreshold(minrssi-22+db);
            QNF_SetRegBit(SYSTEM1,0xA1,0xA0);
            timeout = 0;
            do
            {
                QND_Delay(5);
                regValue = qn8006_gpio_i2c_ReadReg(SYSTEM1);
                timeout++;
            } 
            while((regValue &CHSC)&&timeout < 200);
            if (timeout < 200)
            {
                regValue = qn8006_gpio_i2c_ReadReg(CCA5); 
                ifcnt = regValue;
                regValue = qn8006_gpio_i2c_ReadReg(CCA4); 
                ifcnt =(((ifcnt&0x1f)<<8)+regValue);
                if ((ifcnt < 2268) && (ifcnt > 1828))
                {
                    if (rssi > minrssi + 20 + db)
                    {
                        scanFlag = 0;
                    }
                    else 
                    {
                        QND_Delay(PILOT_READ_OUT_DELAY_TIME);
                        pilot_snr = (qn8006_gpio_i2c_ReadReg(STATUS1) & 0x01) ? 0 : 1;   
                        if (pilot_snr)
                        {
                            scanFlag = 0;
                        }            
                        else
                        {
                            QND_Delay(PILOT_SNR_READ_OUT_DELAY_TIME); 
                            pilot_snr = qn8006_gpio_i2c_ReadReg(SNR);  
                            if (pilot_snr > 5 && pilot_snr < 55) 
                            {
                                scanFlag = 0;
                            }
                        }
                    }
                }
            }
        }
        chfreq = start;
        if (scanFlag)
        {
            start = chfreq + (up ? stepvalue : -stepvalue);
            if ((up && (stop < start)) || (!up && (start < stop)))
            {
                scanFlag = 0;
                chfreq = 0;
            }
        }
        else
        {
            if(qnd_CallBackFunc)
                qnd_CallBackFunc(chfreq, BAND_FM);
        }
    } 
    while (scanFlag != 0);

    if (qnd_AutoScanAll == 0) 
    {
        //Move the filter setting to TunetoChannel
        //QNF_SetRegBit(32,0x08,0x08); 
        QND_TuneToCH((chfreq==0) ? stop: chfreq); 
    }
    //QND_LOG("=== Done QND_AutoScan_FMChannel_One ===");
    return chfreq;  
}

/**********************************************************************
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT16 step, UINT8 db, UINT8 up)
**********************************************************************
Description:    Automatically scans the complete FM or AM band and detects 
            all the available  channels(AM or FM, it will be determine by 
            the workmode which set by QND_SetSysmode). A threshold value 
            needs to be passed in for the channel detection.
Parameters:
    start
        Set the frequency (10kHz) where scan will be started,
        eg: 76.00MHz will be set to 7600.
    stop
        Set the frequency (10kHz) where scan will be stopped,
        eg: 108.00MHz will be set to 10800.
    Step
        FM:
            QND_FMSTEP_100KHZ: set leap step to 100kHz
            QND_FMSTEP_200KHZ: set leap step to 200kHz
            QND_FMSTEP_50KHZ:  set leap step to 50kHz
        AM:
        QND_AMSTEP_***:
    db
        Set signal noise ratio for channel to be searched.
    up:
        Set the seach direction :
        Up;0,seach from stop to start
        Up:1 seach from start to stop

Return Value:
  The channel count found by this function
  -1: no channel found 
**********************************************************************/
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up) 
{
//    UINT16 prevCh = 0;
    //UINT16 chfreq;
    UINT8  stepvalue;
    UINT16 temp;
    UINT16 pStart = start;
    UINT16 pStop = stop > qnd_CH_STOP ? qnd_CH_STOP : stop;
    QNF_SetMute(1);
    up=(start<stop) ? 1 : 0;
    qnd_AutoScanAll = 1;
    stepvalue = qnd_StepTbl[step];
    QNF_SetRegBit(GAIN_SEL,0x38,0x38);
    {
        QND_UpdateRSSIn(0);
    }
    qnd_ChCount = 0;
    do
    {  
        temp = QND_RXSeekCH(pStart, pStop, step, db, up);
        if (temp) 
        {
            qnd_ChList[qnd_ChCount++] =temp;
        }
        else
        {
            temp = pStop;
        }
        {
            pStart = temp + (up ? stepvalue : -stepvalue);
        }
    }
    while((up ? (pStart<=pStop):(pStart>=pStop)) && (qnd_ChCount < QN_CCA_MAX_CH));

    QND_TuneToCH((qnd_ChCount >= 1)? qnd_ChList[0] : pStop); 
    qnd_AutoScanAll = 0;
    QNF_SetMute(0);
    return qnd_ChCount;
}

/************************************************************************
void QND_RXConfigAudio(UINT8 optiontype, UINT8 option )
*************************************************************************
Description: config audio 
Parameters:
  optiontype: option 
    QND_CONFIG_MUTE; \A1\AEoption\A1\AFcontrol muteable, 0:mutedisable,1:mute enable 
    QND_CONFIG_MONO; \A1\AEoption\A1\AFcontrol mono, 0: QND_AUDIO_STEREO,1: QND_AUDIO_STEREO
    QND_CONFIG_EQUALIZER: 'option' control the EQUALIZER,0:disable  EQUALIZER; 1: enable EQUALIZER;
    QND_CONFIG_VOLUME: 'option' control the volume gain,range : 0~83(0: -65db, 65: 0db, 83: +18db
    QND_CONFIG_BASS_QUALITY: 'option' set BASS quality factor,0: 1, 1: 1.25, 2: 1.5, 3: 2
    QND_CONFIG_BASS_FREQ: 'option' set BASS central frequency,0: 60Hz, 1: 70Hz, 2: 80Hz, 3: 100Hz
    QND_CONFIG_BASS_GAIN: 'option' set BASS control gain,range : 0x0~0x1e (00000 :-15db, 11110 :15db
    QND_CONFIG_MID_QUALITY: 'option' set MID quality factor,0 :1, 1 :2
    QND_CONFIG_MID_FREQ: 'option' set MID central frequency,0: 0.5KHz, 1: 1KHz, 2: 1.5KHz, 3: 2KHz
    QND_CONFIG_MID_GAIN: 'option' set MID control gain,range : 0x0~0x1e (00000 :-15db, 11110 :15db)
    QND_CONFIG_TREBLE_FREQ: 'option' set TREBLE central frequency,0: 10KHz, 1: 12.5KHz, 2: 15KHz, 3: 17.5KHz
    QND_CONFIG_TREBLE_GAIN: 'option' set TREBLE control gain,range : 0x0~0x1e (00000 :-15db, 11110 :15db

Return Value:
    none
**********************************************************************/
void QND_RXConfigAudio(UINT8 optiontype, UINT8 option ) 
{
    switch(optiontype)
    {
    case QND_CONFIG_MONO:
        if (option)
            QNF_SetAudioMono(RX_MONO_MASK, QND_RX_AUDIO_MONO);
        else
            QNF_SetAudioMono(RX_MONO_MASK, QND_RX_AUDIO_STEREO);
        break;
    case QND_CONFIG_MUTE:
        if (option)
            QNF_SetMute(1);
        else
            QNF_SetMute(0);
        break;

    default:
        break;
    }
}

/**********************************************************************
UINT16 QND_TXClearChannelScan(UINT16 start, UINT16 stop, UINT16 step,UINT8 db)
**********************************************************************
Description:    Clean channel scan. Finds the best clear channel for transmission.

Parameters:
  Start
    Set the frequency (10kHz) where scan will be started,
    eg: 7600 for 76.00MHz
  Stop
    Set the frequency (10kHz) where scan will be stopped,
    eg: 10800 for 108.00MHz
  Step
    QND_FSTEP_100KHZ: Set leap step to 100kHz.
    QND_FSTEP_200KHZ: Set leap step to 200kHz.
    QND_FSTEP_50KHZ:  Set leap step to 50kHz.
  db
    Set threshold for quality of channel to be searched. 
Return Value:
  The channel frequency (unit: 10kHz)
**********************************************************************/
UINT16 QND_TXClearChannelScan(UINT16 start, UINT16 stop, UINT8 step,UINT8 db)
{
    UINT8 regValue;
    UINT16 chnFreq;
    QNF_ConfigScan(start, stop, step);
    QNF_SetRegBit(CCA, 0xe0, 0x20);  // set TXCCAAqn
    {
        QNF_SetRegBit(SYSTEM1, 0xe1, 0x60);   // &0xfe  |0x60
    }
    do{
        QND_Delay(20);
        regValue =qn8006_gpio_i2c_ReadReg(SYSTEM1);        
    }while(regValue&0x20);
    // suppose it's time to get right TX ch
    chnFreq = QNF_GetCh(); 
    QND_LOGB("BestChannel ",chnFreq);
    return chnFreq;
}

/**********************************************************************
UINT8 QND_TXSetPower( UINT8 gain)
**********************************************************************
Description:    Sets FM transmit power attenuation.

Parameters:
    gain: The transmission power attenuation value, for example, 
          setting the gain = 0x13, TX attenuation will be -6db
          look up table see below
BIT[5:4]
            00    0db
            01    -6db
            10    -12db
            11    -18db
BIT[3:0]    unit: db
            0000    124
            0001    122.5
            0010    121
            0011    119.5
            0100    118
            0101    116.5
            0110    115
            0111    113.5
            1000    112
            1001    110.5
            1010    109
            1011    107.5
            1100    106
            1101    104.5
            1110    103
            1111    101.5
for example:
  0x2f,    //111111    89.5
  0x2e,    //111110    91
  0x2d,    //111101    92.5
  0x2c,    //111100    94
  0x1f,    //111011 95.5
  0x1e,    //111010 97
  0x1d,    //111001 98.5
  0x1c,    //111000 100
  0x0f,    //001111    101.5
  0x0e,    //001110    103
  0x0d,    //001101    104.5
  0x0c,    //001100    106
  0x0b,    //001011    107.5
  0x0a,    //001010    109
  0x09,    //001001    110.5
  0x08,    //001000    112
  0x07,    //000111    113.5
  0x06,    //000110    115
  0x05,    //000101    116.5
  0x04,    //000100    118
  0x03,    //000011    119.5
  0x02,    //000010    121
  0x01,    //000001    122.5
  0x00     //000000    124
**********************************************************************/
void QND_TXSetPower( UINT8 gain)
{
    UINT8 value = 0;
    value |= 0x40;  
    value |= gain;
    qn8006_gpio_i2c_WriteReg(PAG_CAL, value);
}

/**********************************************************************
void QND_TXConfigAudio(UINT8 optiontype, UINT8 option )
**********************************************************************
Description: Config the TX audio (eg: volume,mute,etc)
Parameters:
  optiontype:option :
    QND_CONFIG_AUTOAGC:option'set auto AGC, 0:disable auto AGC,1:enable auto AGC.
    QND_CONFIG_SOFTCLIP;option set softclip,0:disable soft clip, 1:enable softclip.
    QND_CONFIG_MONO;option set mono,0: QND_AUDIO_STEREO, 1: QND_AUDIO_STEREO
    QND_CONFIG_AGCGAIN; option set AGC gain, range:0000~1111
Return Value:
  none
**********************************************************************/
void QND_TXConfigAudio(UINT8 optiontype, UINT8 option )
{
    switch(optiontype)
    {
    case QND_CONFIG_MONO:
        if (option)
            QNF_SetAudioMono(0x10, QND_TX_AUDIO_MONO);
        else
            QNF_SetAudioMono(0x10, QND_TX_AUDIO_STEREO);
        break;

    case QND_CONFIG_MUTE:
        if (option)  
            QNF_SetRegBit(0x28, 0x18,0);
        else    
            QNF_SetRegBit(0x28, 0x18,0x18);
        break;
    case QND_CONFIG_SOFTCLIP:
        if (option)
            QNF_SetRegBit(TXAGC_GAIN,0x80, 0x80);
        else
            QNF_SetRegBit(TXAGC_GAIN,0x80, 0x00);
        break;

    case QND_CONFIG_AUTOAGC:
        if (option)
            QNF_SetRegBit(TXAGC_GAIN,0x40,0);
        else
            QNF_SetRegBit(TXAGC_GAIN,0x41,0x41);
        break;
    case QND_CONFIG_AGCGAIN:
        QNF_SetRegBit(TXAGC_GAIN, 0x1f,option);
        break;
    default:
        break;
    }
}

/**********************************************************************
UINT8 QND_RDSEnable(UINT8 mode) 
**********************************************************************
Description: Enable or disable chip to work with RDS related functions.
Parameters:
          on: QND_RDS_ON:  Enable chip to receive/transmit RDS data.
                QND_RDS_OFF: Disable chip to receive/transmit RDS data.
Return Value:
           QND_SUCCESS: function executed
**********************************************************************/
UINT8 QND_RDSEnable(UINT8 on) 
{
    UINT8 val;

    QND_LOG("=== QND_SetRDSMode === ");
    // get last setting
    val = qn8006_gpio_i2c_ReadReg(SYSTEM1);
    if (on == QND_RDS_ON) 
    {
        val |= RDSEN;
        // if RDS enabled, channel conditional filter is auto selected
        QNF_SetRegBit(GAIN_SEL,0x08,0x00); 
    } 
    else if (on == QND_RDS_OFF)
    {
        val &= ~RDSEN;
        // if RDS disabled, channel conditional filter is set to filter 2
        QNF_SetRegBit(GAIN_SEL,0x38,0x28);
    }
    else 
    {
        return 0;
    }
    qn8006_gpio_i2c_WriteReg(SYSTEM1, val);
    return 1;
}

/**********************************************************************
UINT8 QND_DetectRDSSignal(void)
**********************************************************************
Description: detect the RDSS signal .

Parameters:
    None
Return Value:
    the value of STATUS3
**********************************************************************/
UINT8 QND_RDSDetectSignal(void) 
{
    UINT8 val = qn8006_gpio_i2c_ReadReg(STATUS3);    
    return val;
}

/**********************************************************************
void QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload)
**********************************************************************
Description: Load (TX) or unload (RX) RDS data to on-chip RDS buffer. 
             Before calling this function, always make sure to call the 
             QND_RDSBufferReady function to check that the RDS is capable 
             to load/unload RDS data.
Parameters:
  rdsRawData : 
    8 bytes data buffer to load (on TX mode) or unload (on RXmode) 
    to chip RDS buffer.
  Upload:   
    1-upload
    0--download
Return Value:
    QND_SUCCESS: rds data loaded to/from chip
**********************************************************************/
void QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload) 
{
    UINT8 i;
    UINT8 temp;
    if (upload) 
    {       //TX MODE
        for (i = 0; i <= 7; i++) 
        {
            qn8006_gpio_i2c_WriteReg(RDSD0 + i, rdsRawData[i]);
        }    
    } 
    else 
    {
        //RX MODE
        for (i = 0; i <= 7; i++) 
        {
            temp = qn8006_gpio_i2c_ReadReg(RDSD0 + i);
            rdsRawData[i] = temp;
        }
    }
}

/**********************************************************************
UINT8 QND_RDSCheckBufferReady(void)
**********************************************************************
Description: Check chip RDS register buffer status before doing load/unload of
RDS data.

Parameters:
    None
Return Value:
    QND_RDS_BUFFER_NOT_READY: RDS register buffer is not ready to use.
    QND_RDS_BUFFER_READY: RDS register buffer is ready to use. You can now
    load (for TX) or unload (for RX) data from/to RDS buffer
**********************************************************************/
UINT8 QND_RDSCheckBufferReady(void) 
{
    UINT8 val;
    UINT8 rdsUpdated;
    rdsUpdated = qn8006_gpio_i2c_ReadReg(STATUS3);
    if (qn8006_gpio_i2c_ReadReg(SYSTEM1) & TXREQ) 
    {       
        //TX MODE
        qn8006_gpio_i2c_WriteReg(SYSTEM2,qn8006_gpio_i2c_ReadReg(SYSTEM2)^RDSTXRDY);  
        return QND_RDS_BUFFER_READY;
    }
    do {
        val = qn8006_gpio_i2c_ReadReg(STATUS3)^rdsUpdated;      
    }while(!(val&RDS_RXUPD)) ;
    return QND_RDS_BUFFER_READY;
}

