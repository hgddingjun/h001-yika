/*
 * d.j create 2014.11.27 qn8006 framework
 */
package com.android.server;
 
import android.content.Context;
 
import android.os.IQn8006Service;
 
import android.util.Slog;
 
public class Qn8006Service extends IQn8006Service.Stub {
    private static final String TAG = "Qn8006Service-java: ";
 
    Qn8006Service(){
        nativeQn8006init();
    }
 
    public void setFrequency(int fq){
        nativeSetQn8006Frequency(fq);
    }
 
    public int  getFrequency(){
        return nativeGetQn8006Frequency();
    }
 
    public void setPower(int pwer){
        nativeSetQn8006Power(pwer);
    }
 
    public int  getPower(){
        return nativeGetQn8006Power();
    }
 
    public void setAudioMode(int audmode){
        nativeSetQn8006AudioMode(audmode);
    }
 
    /* here qn8006_audio_config reserved... */
    public void setAudioConfig(int audoptionType, int audoption, int audisRx){
        nativeSetQn8006AudioConfig(audoptionType, audoption, audisRx);
    }
 
    public void setSystemMode(int sysmode){
        nativeSetQn8006SystemMode(sysmode);
    }
 
    public void Init(){
        nativeFmQn8006Init();
    }
 
 
    /* d.j add 2014.12.12 */
    public void apiQn8006csPin(int cs){
        nativeQn8006csPin(cs);
    }
 
    public int apiQn8006getRSSI(int ch){
        return nativeQn8006getRSSI(ch);
    }
 
    public int apiQn8006rxSeekCHAll(int start, int stop, int step, int db, int up){
        return nativeQn8006rxSeekCHAll(start, stop, step, db, up);
    }
 
    public int apiQn8006rxSeekCH(int start, int stop, int step, int db, int up){
        return nativeQn8006rxSeekCH(start, stop, step, db, up);
    }
 
    public void apiQn8006RDSEnable(int en){
        nativeQn8006RDSEnable(en);
    }
 
    public int apiQn8006RDSDetectSignal(){
        return nativeQn8006RDSDetectSignal();
    }
 
    public int apiQn8006RDSCheckBufferReady(){
        return nativeQn8006RDSCheckBufferReady();
    }
 
    public void apiQn8006RDSLoadData(int rdsRawdata[], int len, int isUpload){
        nativeQn8006RDSLoadData(rdsRawdata, len, isUpload);
    }
     
    public void apiQn8006setCountry(int country){
        nativeQn8006setCountry(country);
    }
 
    public void apiQn8006configFMModule(int optionType, int option){
        nativeQn8006configFMModule(optionType, option);
    }
 
    public void apiQn8006loadDefalutSetting(int country){
        nativeQn8006loadDefalutSetting(country);
    }
 
    public int apiQn8006txClearChannelScan(int start, int stop, int step, int db){
        return nativeQn8006txClearChannelScan(start, stop, step, db);
    }
 
    public void apiQn8006powerEnable(int en){
        nativeQn8006powerEnable(en);
    }
 
 
 
 
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
 
    private static native boolean nativeQn8006init();
    private static native void nativeSetQn8006Frequency(int freq);
    private static native int  nativeGetQn8006Frequency();
    private static native void nativeSetQn8006Power(int power);
    private static native int  nativeGetQn8006Power();
    private static native void nativeSetQn8006AudioMode(int audmode);
    private static native void nativeSetQn8006AudioConfig(int audoptionType, int audoption, int audisRx);
    private static native void nativeSetQn8006SystemMode(int sysmode);
    private static native void nativeFmQn8006Init();
    /* d.j add 2014.12.12 */
    private static native void nativeQn8006csPin(int cs);
    private static native int  nativeQn8006getRSSI(int ch);
    private static native int  nativeQn8006rxSeekCHAll(int start, int stop, int step, int db, int up);
    private static native int  nativeQn8006rxSeekCH(int start, int stop, int step, int db, int up);
    private static native void nativeQn8006RDSEnable(int en);
    private static native int  nativeQn8006RDSDetectSignal();
    private static native int  nativeQn8006RDSCheckBufferReady();
    private static native void nativeQn8006RDSLoadData(int rdsRawdata[], int len, int isUpload);
    private static native void nativeQn8006setCountry(int country);
    private static native void nativeQn8006configFMModule(int optionType, int option);
    private static native void nativeQn8006loadDefalutSetting(int country);
    private static native int  nativeQn8006txClearChannelScan(int start, int stop, int step, int db);
    private static native void nativeQn8006powerEnable(int en);
};