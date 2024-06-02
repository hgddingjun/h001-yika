/*
 * d.j create 2014.11.27
 */
 
package android.os;
 
interface IQn8006Service {
     void setFrequency(int fq);
     int  getFrequency();
     void setPower(int pwer);
     int  getPower();
     void setAudioMode(int audmode);
     void setAudioConfig(int audoptionType, int audoption, int audisRx);
     void setSystemMode(int sysmode);
     void Init();
     void apiQn8006csPin(int cs);
     int apiQn8006getRSSI(int ch);
     int apiQn8006rxSeekCHAll(int start, int stop, int step, int db, int up);
     int apiQn8006rxSeekCH(int start, int stop, int step, int db, int up);
     void apiQn8006RDSEnable(int en);
     int apiQn8006RDSDetectSignal();
     int apiQn8006RDSCheckBufferReady();
     void apiQn8006RDSLoadData(in int [] rdsRawdata, int len, int isUpload);
     void apiQn8006setCountry(int country);
     void apiQn8006configFMModule(int optionType, int option);
     void apiQn8006loadDefalutSetting(int country);
     int apiQn8006txClearChannelScan(int start, int stop, int step, int db);
     void apiQn8006powerEnable(int en);
 }
