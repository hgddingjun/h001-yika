
//#define LOG_NDEBUG 0
//#define LOG_TAG "emBtTest-JNI"
//#include "utils/Log.h"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include <pthread.h>
//#include <utils/threads.h>
#include <dlfcn.h>
#include <string.h>
#include "jni.h"
//#include "JNIHelp.h"
//#include "android_runtime/AndroidRuntime.h"
//#include "utils/Errors.h"  // for status_t
#undef LOG_NDEBUG 
#undef NDEBUG

#ifdef LOG_TAG
#undef LOG_TAG
#define TAG "emBtTest-JNI"
#endif

#include "bt_em.h"
#define BUFER_SIZE 1024*8
//#include <cutils/xlog.h>
//#include <cutils/sockets.h>
//#include <cutils/properties.h>
//typedef int BOOL;
#define TAG "emBtTest-JNI"
#include "android/log.h"
#define XLOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define XLOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define XLOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)
#define LOGW(fmt, args...) __android_log_print(ANDROID_LOG_WARN, TAG, fmt, ##args)

#define ERR(f, ...)       LOGE("%s:%d " f, __func__,__LINE__, ##__VA_ARGS__)
#define WAN(f, ...)       LOGD("%s:%d " f, __func__,__LINE__, ##__VA_ARGS__)

// ----------------------------------------------------------------------------

//using namespace android;

// ----------------------------------------------------------------------------

struct fields_t {
	jfieldID patter;
	jfieldID channels;
	jfieldID pocketType;
	jfieldID pocketTypeLen;
	jfieldID freq;
	jfieldID power;
};
static fields_t fields;

static unsigned char Pattern_Map[] = { 0x01, 0x02, 0x03, 0x04, 0x09 };
static unsigned char PocketType_Map[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0E, 0x0F, 0x17, 0x1C, 0x1D, 0x24,
		0x28, 0x2A, 0x2B, 0x2E, 0x2F, 0x36, 0x37, 0x3C, 0x3D };
static unsigned char Channels_Map[] = { 0x00, 0x01 };

struct fields_ts{
	jfieldID tty;
	jmethodID post_event;
};
static fields_ts fieldst;
JavaVM* g_JavaVM;

#define RECEIVE_DATA_INDEX (1)
#define SEND_DATA_INDEX (2)
#define POST_EVENT()
extern "C" int uart_fd;
 int mOpen = 0;

 struct btpack{
	 char strCommand[6];		//命令标识
	 unsigned char clen;		//返回数据长度
	 unsigned char content[32];	//返回数据结果
 };

/**
* class Listener
*/
class JNIMyObserver{
	public:
		JNIMyObserver(JNIEnv* env,jobject thiz,jobject weak_thiz);
		~JNIMyObserver();
		void OnEvent(const char* buffer,int length,int what);
		int prepacketDeal(const char *buffer, int length, int what);	//整条命令识别
		int precheckPacket(const char *buffer, int length);				//包命令校验 查看包是否是 <>对命令
		int parserespPacket(const char *buffer, int length);			//解析命令包
	private:
		JNIMyObserver();
		btpack mPacket;
		jclass mClass;
		jobject mObject;
		pthread_mutex_t mutex;
		unsigned char curPos;
		unsigned char buf[64];
};
JNIMyObserver::JNIMyObserver(JNIEnv* env,jobject thiz,jobject weak_thiz){
	jclass clazz = env->GetObjectClass(thiz);
    //初始化互斥量
    if (0 != pthread_mutex_init(&mutex, NULL)) {
        //异常
        jclass exceptionClazz = env->FindClass("java/lang/RuntimeException");
        //抛出
        env->ThrowNew(exceptionClazz, "Unable to init mutex--");
    }
	if(clazz == NULL){
	// jniThrowException(env,"java/lang/Exception",NULL);
		LOGE("clazz is null");
		return;
	}
	memset(buf, 0, 64);
	curPos = 0;
	mClass = (jclass)env->NewGlobalRef(clazz);
	mObject = env->NewGlobalRef(weak_thiz);
//	LOGW("mClass=%d",mClass);
//	LOGW("mObject=%d",mObject);
}
JNIMyObserver::~JNIMyObserver(){
	JNIEnv* env;
	bool status = (g_JavaVM->GetEnv((void**) &env,JNI_VERSION_1_4) != JNI_OK);
	if(status){
		g_JavaVM->AttachCurrentThread(&env,NULL);
	}
    if (0 != pthread_mutex_destroy(&mutex)) {
        //异常
        jclass exceptionClazz = env->FindClass("java/lang/RuntimeException");
        //抛出
        env->ThrowNew(exceptionClazz, "Unable to destroy mutex--");
    }
	env->DeleteGlobalRef(mObject);
	env->DeleteGlobalRef(mClass);
}
int JNIMyObserver::parserespPacket(const char* buffer,int length){
	int ret = 0;
	int i=0;
	if(buffer && length > 3){

		//找到开始位置
		while(i < length	){
			if(buffer[i] == '['||buffer[i] == '@'||buffer[i] == '#' )	break;
			i++;
		}
		if(i < 5 && i > 1){	//长度不能太长
			if(buffer[i] == '['){
				memcpy(mPacket.strCommand, buffer, i-1);
				memcpy(mPacket.content, buffer+i, length-i);
				ret = length - i;
			}else{
				memcpy(mPacket.strCommand, buffer+1, i-1);
				i++;
				memcpy(mPacket.content, buffer+i, length-i-2);
				ret = length - i -2;
			}
			mPacket.strCommand[i-1] = '\0';
			mPacket.content[ret] = '\0';
			WAN("cmd = %s, content = %s", mPacket.strCommand,mPacket.content);
		}
	}
	WAN("%s, ret = %d, length = %d, i = %d",buffer,  ret, length, i);
	return ret;
}
int JNIMyObserver::precheckPacket(const char* buffer,int length){
	int ret = 1;
	if(buffer && length > 3){
		if(buffer[0] == '<' && buffer[length-3] == '>'){
			ret = 1;
		}
	}
	WAN("ret = %d, length = %d", ret, length);
	return ret;
}
int JNIMyObserver::prepacketDeal(const char* buffer,int length,int what){
	//
	int ret = 0;
	if(buffer && length >0){
		if(curPos + length <64){
			int i =0 ;
			memcpy(buf+curPos, buffer, length);
			while(i < length-1){
				if(buffer[i] == 0x0d && buffer[i+1] == 0x0a) break;
				i++;
			}
			if(i < length-1) ret = curPos + i+2;
			curPos += length;
		}
	}
	WAN("ret = %d, curPos=%d", ret, curPos);
	return ret;
}

void JNIMyObserver::OnEvent(const char* dat,int len,int what){
	/*
	* 鍒涘缓env鎸囬拡
	*/
	JNIEnv* env;
	//判断是否满了一个数据包
	int ret = len;
	WAN("len = %d", len);
	{	//收到帧数据了
		char buffer[64] ;
		int length;
		length = ret;
		memcpy(buffer, dat, ret);

		bool status = (g_JavaVM->GetEnv((void**) &env,JNI_VERSION_1_4) != JNI_OK);
		if(status){
			g_JavaVM->AttachCurrentThread(&env,NULL);
		}

		if (0 != pthread_mutex_lock(&mutex)) {
			//异常
			jclass exceptionClazz = env->FindClass("java/lang/RuntimeException");
			//抛出
			env->ThrowNew(exceptionClazz, "Unable to lock mutex--");
			return;
		}

		if(NULL == g_JavaVM){
			LOGE("JNIObserver::Event g_JavaVM is NULL");
			return;
		}
		bool isAttacked = false;

		/*
		* 鍒涘缓JAVA byte[]鏁扮粍
		*/
		jbyteArray obj = NULL;
		if(buffer !=NULL && buffer != 0){
			const jbyte* data = reinterpret_cast<const jbyte*>(buffer);
			obj = env->NewByteArray(length);
			env->SetByteArrayRegion(obj,0,length,data);
		}

		env->CallStaticVoidMethod(mClass,fieldst.post_event,mObject,what,0,0,obj);
		if(obj){
			env->DeleteLocalRef(obj);
		}

		if(isAttacked){
			g_JavaVM->DetachCurrentThread();//鍒嗙绾跨▼
		}
		WAN("OnEvent");
		//unlock
		if (0 != pthread_mutex_unlock(&mutex)) {
			//异常
			jclass exceptionClazz = env->FindClass("java/lang/RuntimeException");
			//抛出
			env->ThrowNew(exceptionClazz, "Unable to unlock mutex--");
		}
	}
}



/**
* class Listener end -----------------
*/
JNIMyObserver* listener;
/*
* setup
*/
static void native_uartEvent(JNIEnv *env,jobject clazz,jobject weak_this){
	LOGW("native_uartEvent");
	env->GetJavaVM(&g_JavaVM);
	if(listener != NULL){
		delete listener;
	}
	listener = new JNIMyObserver(env,clazz,weak_this);
}

/*
* 线程Run
*/
void* threadreadTtyData(void* arg){
	LOGW("run read data");
	if(!(arg)){
		return NULL;
	}
	LOGW("run read data");
	char* buf = new char[200];
	int result = 0,ret;
	fd_set readfd;
	struct timeval timeout;
	while(mOpen){
		timeout.tv_sec = 2;//设定超时秒数
		timeout.tv_usec = 0;//设定超时毫秒数
		FD_ZERO(&readfd);//清空集合
		FD_SET(uart_fd,&readfd);///* 把要检测的句柄mTtyfd加入到集合里 */
		ret = select(uart_fd+1,&readfd,NULL,NULL,&timeout);/* 检测我们上面设置到集合readfd里的句柄是否有可读信息 */
		switch(ret){
			case -1:/* 这说明select函数出错 */
				result = -1;
				LOGE("mTtyfd read failure");
			break;
			case 0:/*说明在我们设定的时间值5秒加0毫秒的时间内，mTty的状态没有发生变化*/
				break;
			default:/* 说明等待时间还未到5秒加0毫秒，mTty的状态发生了变化 */
				if(FD_ISSET(uart_fd,&readfd)){/* 先判断一下mTty这外被监视的句柄是否真的变成可读的了 */
					int len = read(uart_fd,buf,sizeof(buf));
					/**发送数据**/
					if(!(arg))break;
					JNIMyObserver *l = static_cast<JNIMyObserver *>(arg);
					l->OnEvent(buf,len,RECEIVE_DATA_INDEX);
					memset(buf,0,sizeof(buf));
				}
		break;
		}
		if(result == -1){
			break;
		}
	}
	if(buf != NULL){
		delete buf;
		buf = NULL;
	}
	LOGE("stop run!");
	return NULL;
}
/*
* _receiveMsgFromTty
*/
static int native_listenUart(JNIEnv *env,jobject clazz){
	LOGW("native_listenUart");
	if(uart_fd < 0){
		LOGE("uart_fd open failure ,non't read");
		return -1;
	}
	pthread_t id;
	int ret;
	ret = pthread_create(&id,NULL,threadreadTtyData,listener);
	if(ret != 0){
		LOGE("create receiver thread failure ");
	}else{
		LOGW("create read data thred success");
	}
	return ret;
}
static int BtTest_relayerStart(JNIEnv *env, jobject thiz , jint portNumber, jint serialSpeed) {
	XLOGI("enter RELAYER_start serialSpeed =%d portNumber =%d\n", serialSpeed, portNumber);
	return RELAYER_start(portNumber, serialSpeed) ? 0 : -1;
}
static int BtTest_open(JNIEnv *env, jobject thiz , jint portNumber, jint serialSpeed) {
	XLOGI("enter BtTest_open serialSpeed =%d portNumber =%d\n", serialSpeed, portNumber);
	return RELAYER_open(portNumber, serialSpeed) ;
}
static int BtTest_close(JNIEnv *env, jobject thiz) {
	XLOGI("enter BtTest_close\n");
	RELAYER_close();
	return 0;
}
static int BtTest_relayerExit(JNIEnv *env, jobject thiz) {
	XLOGI("enter RELAYER_exit\n");
	RELAYER_exit();
	return 0;
}
static int BtTest_sendstr(JNIEnv *env, jobject thiz, jstring buffer) {
	XLOGI("enter send\n");
	int ret = 0;
	int len =0;
	if (buffer) {
		jchar* jniParameter = NULL;
		jsize      strLen = 0;//
		jniParameter = (jchar *) env->GetStringUTFChars(buffer, JNI_FALSE);;
		if (jniParameter == NULL) {
			XLOGE("Error retrieving source of EM paramters");
			return 0; // out of memory or no data to load
		}else{
			strLen = strlen((char*)jniParameter);
			len = strlen((char*)jniParameter);
			XLOGE("Native send = %d: %s \n",strLen, (char*)jniParameter);
		}
		if(len > 0)
		{
//			if(listener !=NULL){
//				listener->OnEvent((char*)jniParameter,len,SEND_DATA_INDEX);
//			}
			//len = (int)strlen ;
			ERR("malloc failed 0 len= %d \n", len);
			unsigned char *pb=NULL;
			pb = (unsigned char *)malloc(len+4);
			if(pb){
				ERR("malloc failed 1 len= %d \n", len);
				memset((void*)pb, 0, len+4);
				ERR("malloc failed 2\n");
				memcpy(pb, (unsigned char *)jniParameter, len);
				ERR("malloc failed 3\n");
				pb[len++] = 0x0d;
				pb[len++] = 0x0a;
				ret = send(pb, len);
				free(pb);
			}
			else{
				ERR("malloc failed \n");
			}
		}
		env->ReleaseStringUTFChars(buffer, (char*)jniParameter);
	} else {
		XLOGE("NULL java array of readEEPRom16");
		return 0;
	}
	return ret;
}
static int BtTest_senddata(JNIEnv *env, jobject thiz, jbyteArray buffer, jint start, jint tlen) {
	XLOGI("enter send\n");
	int ret = 0;
	int len =0;
	if (buffer) {
		jbyte * olddata = (jbyte*)env->GetByteArrayElements(buffer, 0);
		jsize  oldsize = env->GetArrayLength(buffer);
		unsigned char * bytearr = (unsigned char*)olddata;
		len = (int)oldsize;
		//长度判断
		if (bytearr == NULL || len <= start + tlen|| tlen< 1) {
			XLOGE("Error retrieving source of EM paramters");
			env->ReleaseByteArrayElements(buffer, olddata, 0);
			return 0; // out of memory or no data to load
		}else{
			XLOGE("Native send = %d: \n",len);
		}
		if(tlen > 0)
		{
			len = tlen;
			ERR("malloc failed 0 len= %d \n", len);
			unsigned char *pb=NULL;
			pb = (unsigned char *)malloc(len+4);
			if(pb){
				memset((void*)pb, 0, len+4);
				memcpy(pb, (unsigned char *)bytearr+start, len);
				pb[len++] = 0x0d;
				pb[len++] = 0x0a;
				ret = send(pb, len);
				free(pb);
			}
			else{
				ERR("malloc failed \n");
			}
		}
		env->ReleaseByteArrayElements(buffer, olddata, 0);
	//	env->ReleaseStringUTFChars(buffer, (char*)jniParameter);
	} else {
		XLOGE("NULL java array of readEEPRom16");
		return 0;
	}
	return ret;
}
static int BtTest_sendbin(JNIEnv *env, jobject thiz, jbyteArray buffer) {
	XLOGI("enter send\n");
	int ret = 0;
	int len =0;
	if (buffer) {
		jbyte * olddata = (jbyte*)env->GetByteArrayElements(buffer, 0);
		jsize  oldsize = env->GetArrayLength(buffer);
		unsigned char * bytearr = (unsigned char*)olddata;
		len = (int)oldsize;

		if (bytearr == NULL) {
			XLOGE("Error retrieving source of EM paramters");
			return 0; // out of memory or no data to load
		}else{
			XLOGE("Native send = %d: \n",len);
		}
		if(len > 0)
		{
			ERR("malloc failed 0 len= %d \n", len);
			unsigned char *pb=NULL;
			pb = (unsigned char *)malloc(len+4);
			if(pb){
				memset((void*)pb, 0, len+4);
				memcpy(pb, (unsigned char *)bytearr, len);
				pb[len++] = 0x0d;
				pb[len++] = 0x0a;
				ret = send(pb, len);
				free(pb);
			}
			else{
				ERR("malloc failed \n");
			}
		}
		env->ReleaseByteArrayElements(buffer, olddata, 0);
	//	env->ReleaseStringUTFChars(buffer, (char*)jniParameter);
	} else {
		XLOGE("NULL java array of readEEPRom16");
		return 0;
	}
	return ret;
}
static jstring BtTest_exec(JNIEnv* env,jobject thiz, jstring cmd)
{
	if(cmd !=NULL){
		char* str2;
		str2=(char*)env->GetStringUTFChars(cmd, JNI_FALSE);
		if(str2){
			FILE *fp;
			char buffer[BUFER_SIZE] = {0};
			memset(buffer,0,BUFER_SIZE);
			fp=popen(str2,"r");
			env->ReleaseStringUTFChars( cmd, str2);
			if(fp){
				fread(buffer, BUFER_SIZE-1, 1, fp);
				buffer[BUFER_SIZE-1] = 0;
				pclose(fp);
				return env->NewStringUTF(buffer);
			}
		}
	}
	return NULL;
}
static JNINativeMethod mehods[] = {
//		{ "getChipId", "()I",(void *) BtTest_getChipId },
//		{ "isBLESupport", "()I",(void *) BtTest_isBLESupport },
//		{ "init", "()I", (void *) BtTest_Init },
//		{ "doBtTest", "(I)I",(void *) BtTest_doBtTest },
//		{ "unInit", "()I",(void *) BtTest_UnInit },
//		{ "hciCommandRun", "([CI)[C",(void *) BtTest_HCICommandRun },
//		{ "hciReadEvent", "()[C",(void *) BtTest_HCIReadEvent },
//
//		{ "noSigRxTestStart", "(IIII)Z", (void *) BtTest_StartNoSigRxTest },
//		{ "noSigRxTestResult", "()[I", (void *) BtTest_EndNoSigRxTest },
//
//		{"getChipIdInt", "()I", (void *) BtTest_GetChipIdInt },
//		{"getChipEcoNum", "()I", (void *) BtTest_GetChipEcoNum },
//		{"getPatchId", "()[C", (void *) BtTest_GetPatchId },
//		{"getPatchLen", "()J", (void *) BtTest_GetPatchLen },
		{"open", "(II)I", (void *) BtTest_open },
		{"close", "()V", (void *) BtTest_close },
		{"relayerStart", "(II)I", (void *) BtTest_relayerStart },
		{"relayerExit", "()I", (void *) BtTest_relayerExit },
		{"sendstr", "(Ljava/lang/String;)I", (void *) BtTest_sendstr },
		{"sendbin", "([B)I", (void *) BtTest_sendbin },
		{"senddata", "([BII)I", (void *) BtTest_senddata },
		{"exec", "(Ljava/lang/String;)Ljava/lang/String;", (void *)BtTest_exec},
		{"listenUart", "()I", (void *) native_listenUart },
		{"uartEvent","(Ljava/lang/Object;)V",(void*)native_uartEvent},
//		{"setFWDump", "(J)I", (void *)BtTest_setFWDump },
//		{"isComboSupport","()I", (void *)BtTest_isComboSupport },
//		{"pollingStart","()I", (void*)BtTest_pollingStart },
//		{"pollingStop", "()I", (void*)BtTest_pollingStop },
//		{"setSSPDebugMode", "(Z)Z", (void*)BtTest_setSSPDebugMode},
};

static const char *className = "com/yeahka/letcar/utils/dev/carDev";

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif
jint JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env = NULL;
    jclass clazz = NULL;
    jint result = -1;
    
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        XLOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    
    assert(env != NULL);
    clazz = env->FindClass(className);
	fieldst.post_event = env->GetStaticMethodID(clazz,"onUartRecv","(Ljava/lang/Object;IIILjava/lang/Object;)V");
	if(fieldst.post_event == NULL){
		LOGE("Can't find com/yeahka/letcar/utils/dev/carDev.onUartRecv");
		goto bail;
	}
    if (clazz == NULL) {
        XLOGE("Native registration unable to find class: %s \n", className);
        goto bail;
    }
    
    if (env->RegisterNatives(clazz, mehods, NELEM(mehods)) < 0) {
        XLOGE("ERROR: native registration failed \n");
        goto bail;
    }
    
    result = JNI_VERSION_1_4;
    
bail:
    return result;
}

