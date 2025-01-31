/*
 * d.j create 2014.11.26 for qn8006 jni
 */
#define LOG_TAG "Qn8006Service"
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include <cutils/log.h>
#include <utils/misc.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <hardware/qn8006cdev.h>

namespace android
{
   struct qn8006_device_t *qn8006_device = NULL;
 
    /* begin interface encapsulation */
 
    static void qn8006_setFrequency(JNIEnv *env, jobject clazz, jint fq){
        int valfq = fq;
        ALOGI("Qn8006 JNI: qn8006_setFrequency frequency=0x%x write to device.", valfq);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->set_frequency(qn8006_device, valfq);
    }
 
    static jint qn8006_getFrequency(JNIEnv *env, jobject clazz){
        int valfq=0;
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        qn8006_device->get_frequency(qn8006_device, &valfq);
 
        ALOGI("Qn8006 JNI: qn8006_getFrequency frequency=0x%x get from device.", valfq);
         
        return valfq;
    }
 
 
    static void qn8006_setPower(JNIEnv *env, jobject clazz, jint pwer){
        int valpwer = pwer;
        ALOGI("Qn8006 JNI: qn8006_setPower power=0x%x write to device.", valpwer);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->set_power(qn8006_device, valpwer);
    }
 
    static jint qn8006_getPower(JNIEnv *env, jobject clazz){
        int valpwer=0;
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        qn8006_device->get_power(qn8006_device, &valpwer);
 
        ALOGI("Qn8006 JNI: qn8006_getPower power=0x%x get from device.", valpwer);
         
        return valpwer;
    }
 
 
 
    static void qn8006_setAudioMode(JNIEnv *env, jobject clazz, jint mode){
        int audMode = mode;
        ALOGI("Qn8006 JNI: qn8006_setAudioMode audMode=0x%x write to device.", audMode);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->audio_mode(qn8006_device, audMode);
    }
 
    /* qn8006_audio_config reserved... */
 
    static void qn8006_setAudioConfig(JNIEnv *env, jobject clazz, jint optType, jint opt, jint isrx){
        int audoptType = optType;
        int audopt = opt;
        int audisrx = isrx;
        ALOGI("Qn8006 JNI: qn8006_setAudioConfig optiontype=0x%x, option=0x%x, isRx=0x%x write to device.", 
                         optType, opt, isrx);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->audio_config(qn8006_device, audoptType, audopt, audisrx);
    }    
 
     
    static void qn8006_setSystemMode(JNIEnv *env, jobject clazz, jint mode){
        int sysmode = mode;
        ALOGI("Qn8006 JNI: qn8006_setSystemMode sysmode=0x%x write to device.", sysmode);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->system_mode(qn8006_device, sysmode);
    }
 
 
    static void qn8006_Init(JNIEnv *env, jobject clazz){
 
        ALOGI("Qn8006 JNI: qn8006_Init.");
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->init(qn8006_device);
    }
 
 
        static void com_android_server_qn8006_csPin(JNIEnv *env, jobject clazz, jint level){
        int cspin = level;
        ALOGI("Qn8006 JNI: com_android_server_qn8006_csPin cspin=0x%x write to device.", cspin);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->csPin(qn8006_device, cspin);
    }
 
        static int com_android_server_qn8006_getRSSI(JNIEnv *env, jobject clazz, jint ch){
        int retrssi=0;
        int rssiCH = ch;
         
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        retrssi = qn8006_device->getRSSI(qn8006_device, rssiCH);
        ALOGI("Qn8006 JNI: com_android_server_qn8006_getRSSI rssiCH=0x%x write to device, retrssi=0x%x.", rssiCH, retrssi);
        return retrssi;
    }
 
 
 
        static int com_android_server_qn8006_rxSeekCHAll(JNIEnv *env, jobject clazz, jint start, jint stop, jint step, jint db, jint up){
        int retval=0;
         
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        retval = qn8006_device->rxSeekCHAll(qn8006_device, (int)start, (int)stop, (int)step, (int)db, (int)up);
        ALOGI("Qn8006 JNI: com_android_server_qn8006_rxSeekCHAll retval=0x%x.", retval);
        return retval;
    }
 
 
        static int com_android_server_qn8006_rxSeekCH(JNIEnv *env, jobject clazz, jint start, jint stop, jint step, jint db, jint up){
        int retval=0;
         
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        retval = qn8006_device->rxSeekCH(qn8006_device, (int)start, (int)stop, (int)step, (int)db, (int)up);
        ALOGI("Qn8006 JNI: com_android_server_qn8006_rxSeekCH retval=0x%x.", retval);
        return retval;
    }
 
        static void com_android_server_qn8006_RDSEnable(JNIEnv *env, jobject clazz, jint en){
        int rdsen = en;
        ALOGI("Qn8006 JNI: com_android_server_qn8006_RDSEnable rdsen=0x%x write to device.", rdsen);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->RDSEnable(qn8006_device, rdsen);
    }
 
    static jint com_android_server_qn8006_RDSDetectSignal(JNIEnv *env, jobject clazz){
        int dtSignal=0;
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        qn8006_device->RDSDetectSignal(qn8006_device, &dtSignal);
 
        ALOGI("Qn8006 JNI: com_android_server_qn8006_RDSDetectSignal dtSignal=0x%x get from device.", dtSignal);
         
        return dtSignal;
    }
 
 
 
    static jint com_android_server_qn8006_RDSCheckBufferReady(JNIEnv *env, jobject clazz){
        int chkbufrdy=0;
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        qn8006_device->RDSCheckBufferReady(qn8006_device, &chkbufrdy);
 
        ALOGI("Qn8006 JNI: com_android_server_qn8006_RDSCheckBufferReady chkbufrdy=0x%x get from device.", chkbufrdy);
         
        return chkbufrdy;
    }
 
    static void com_android_server_qn8006_RDSLoadData(JNIEnv *env, jobject clazz, jintArray rdsData, jint len, jint isUpload){
 
        int tmpArray[8];
         
        ALOGI("Qn8006 JNI: com_android_server_qn8006_RDSLoadData isUpload=0x%x write to device.", isUpload);
 
        env->GetIntArrayRegion(rdsData, 0, len, tmpArray);
  
        //ALOGI("Qn8006 JNI: RDSLoadData rdsData[0]=0x%x, rdsData[1]=0x%x, rdsData[2]=0x%x, rdsData[3]=0x%x, rdsData[4]=0x%x, rdsData[5]=0x%x, rdsData[6]=0x%x, rdsData[7]=0x%x write to device.", rdsData[0], rdsData[1], rdsData[2], rdsData[3], rdsData[4], rdsData[5], rdsData[6], rdsData[7]);
 
       ALOGI("Qn8006 JNI: RDSLoadData tmpArray[0]=0x%x, tmpArray[1]=0x%x, tmpArray[2]=0x%x, tmpArray[3]=0x%x, tmpArray[4]=0x%x, tmpArray[5]=0x%x, tmpArray[6]=0x%x, tmpArray[7]=0x%x write to device.", tmpArray[0], tmpArray[1], tmpArray[2], tmpArray[3], tmpArray[4], tmpArray[5], tmpArray[6], tmpArray[7]);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->RDSLoadData(qn8006_device, tmpArray, len, isUpload);
    }
 
 
        static void com_android_server_qn8006_setCountry(JNIEnv *env, jobject clazz, jint cty){
        int country = cty;
        ALOGI("Qn8006 JNI: com_android_server_qn8006_setCountry country=0x%x write to device.", country);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->setCountry(qn8006_device, country);
    }
 
    static void com_android_server_qn8006_configFMModule(JNIEnv *env, jobject clazz, jint optType, jint opt){
        int optionType = optType;
        int option = opt;
 
        ALOGI("Qn8006 JNI: com_android_server_qn8006_configFMModule optiontype=0x%x, option=0x%x write to device.", optionType, option);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->configFMModule(qn8006_device, optionType, option);
    }   
 
        static void com_android_server_qn8006_loadDefalutSetting(JNIEnv *env, jobject clazz, jint cty){
        int country = cty;
        ALOGI("Qn8006 JNI: com_android_server_qn8006_loadDefalutSetting country=0x%x write to device.", country);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->loadDefalutSetting(qn8006_device, country);
    }
 
static int com_android_server_qn8006_txClearChannelScan(JNIEnv *env, jobject clazz, jint start, jint stop, jint step, jint db){
        int retval=0;
         
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return -1;
        }
 
        retval = qn8006_device->txClearChannelScan(qn8006_device, (int)start, (int)stop, (int)step, (int)db);
        ALOGI("Qn8006 JNI: com_android_server_qn8006_txClearChannelScan retval=0x%x.", retval);
        return retval;
    }
 
        static void com_android_server_qn8006_powerEnable(JNIEnv *env, jobject clazz, jint en){
        int powerEn = en;
        ALOGI("Qn8006 JNI: com_android_server_qn8006_powerEnable powerEn=0x%x write to device.", powerEn);
        if(NULL == qn8006_device)
        {
            ALOGE("Qn8006 JNI: qn8006 device can not open, ERR!!");
            return;
        }
 
        qn8006_device->powerEnable(qn8006_device, powerEn);
    }
 
    /* end interface encapsulation */
 
 
 
   /* ^!^ */
   static inline int qn8006_device_jni_open(const struct hw_module_t *module, struct qn8006_device_t **device){
      return module->methods->open( module, QN8006_HARDWARE_MODULE_ID, (struct hw_device_t **)device );
   }
 
   static jboolean qn8006_jni_init(JNIEnv *env, jobject clazz)
   {
      struct qn8006_module_t *module;
      ALOGI("Qn8006 JNI: qn8006_jni_init...");
      if( 0 == hw_get_module(QN8006_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module) )
      {
          ALOGI("Qn8006 JNI: Qn8006 Stub have been found, OK!");
          if( 0 == qn8006_device_jni_open( &(module->common), &qn8006_device) )
          {
              ALOGI("Qn8006 JNI: Qn8006 device is open, OK!");
              return 0;
          }
          ALOGI("Qn8006 JNI: Qn8006 device is failed open ERR :(");
          return -1;
      }
      ALOGI("Qn8006 JNI: Get Qn8006 Stub failed ERR :(");
      return -1;
   }
 
 
 
   /* JNI method tables */
   static const JNINativeMethod method_table[]={
        /* name,                          signature,     funcPtr */
       {"nativeQn8006init",                "()Z" ,       (void *) qn8006_jni_init},
       {"nativeSetQn8006Frequency",        "(I)V" ,      (void *) qn8006_setFrequency},
       {"nativeGetQn8006Frequency",        "()I" ,       (void *) qn8006_getFrequency},
       {"nativeSetQn8006Power",            "(I)V" ,      (void *) qn8006_setPower},
       {"nativeGetQn8006Power",            "()I" ,       (void *) qn8006_getPower},
       {"nativeSetQn8006AudioMode",        "(I)V" ,      (void *) qn8006_setAudioMode},
       {"nativeSetQn8006AudioConfig",      "(III)V" ,    (void *) qn8006_setAudioConfig},
       {"nativeSetQn8006SystemMode",       "(I)V" ,      (void *) qn8006_setSystemMode},
       {"nativeFmQn8006Init",              "()V" ,       (void *) qn8006_Init},
        /* d.j add 2014.12.12 */
       {"nativeQn8006csPin",               "(I)V",       (void *)com_android_server_qn8006_csPin},
       {"nativeQn8006getRSSI",             "(I)I",       (void *)com_android_server_qn8006_getRSSI},
       {"nativeQn8006rxSeekCHAll",         "(IIIII)I",   (void *)com_android_server_qn8006_rxSeekCHAll},
       {"nativeQn8006rxSeekCH",            "(IIIII)I",   (void *)com_android_server_qn8006_rxSeekCH},
       {"nativeQn8006RDSEnable",           "(I)V",       (void *)com_android_server_qn8006_RDSEnable},
       {"nativeQn8006RDSDetectSignal",     "()I",        (void *)com_android_server_qn8006_RDSDetectSignal},
       {"nativeQn8006RDSCheckBufferReady", "()I",        (void *)com_android_server_qn8006_RDSCheckBufferReady},
       {"nativeQn8006RDSLoadData",         "([III)V",     (void *)com_android_server_qn8006_RDSLoadData},
       {"nativeQn8006setCountry",          "(I)V",       (void *)com_android_server_qn8006_setCountry},
       {"nativeQn8006configFMModule",      "(II)V",      (void *)com_android_server_qn8006_configFMModule},
       {"nativeQn8006loadDefalutSetting",  "(I)V",       (void *)com_android_server_qn8006_loadDefalutSetting},
       {"nativeQn8006txClearChannelScan",  "(IIII)I",    (void *)com_android_server_qn8006_txClearChannelScan},
           {"nativeQn8006powerEnable",         "(I)V",       (void *)com_android_server_qn8006_powerEnable},
   };
 
   /* register JNI method */
   int register_android_server_Qn8006Service(JNIEnv *env)
   {
       return jniRegisterNativeMethods(env, "com/android/server/Qn8006Service", method_table, NELEM(method_table));
   }
 
};