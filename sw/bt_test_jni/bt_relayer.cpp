
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>

#include "bt_em.h"

typedef unsigned long DWORD;
typedef unsigned long* PDWORD;
typedef unsigned long* LPDWORD;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef unsigned long HANDLE;
typedef void VOID;
typedef void* LPCVOID;
typedef void* LPVOID;
typedef void* LPOVERLAPPED;
typedef unsigned char* PUCHAR;
typedef unsigned char* PBYTE;
typedef unsigned char* LPBYTE;

#define TRUE           1
#define FALSE          0


#define TAG           "BT_RELAYER "
#include "android/log.h"
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)
#define ERR(f, ...)       LOGE("%s: " f, __func__, ##__VA_ARGS__)
#define WAN(f, ...)       LOGD("%s: " f, __func__, ##__VA_ARGS__)

//#define ERR(f, ...)       LOGE("%s: " f, __func__, ##__VA_ARGS__)
//#define WAN(f, ...)       LOGD("%s: " f, __func__, ##__VA_ARGS__)

//#include <cutils/log.h>
//
//#define BT_RELAYER_DEBUG  1
//#define ERR(f, ...)       ALOGE("%s: " f, __func__, ##__VA_ARGS__)
//#define WAN(f, ...)       ALOGW("%s: " f, __func__, ##__VA_ARGS__)
//#if BT_RELAYER_DEBUG
//#define DBG(f, ...)       ALOGD("%s: " f, __func__, ##__VA_ARGS__)
//#define TRC(f)            ALOGW("%s #%d", __func__, __LINE__)
//#else
#define DBG(...)          ((void)0)
#define TRC(f)            ((void)0)
//#endif

//===============        Global Variables         =======================

 int uart_fd = -1;
extern "C"  int mOpen;
static pthread_t txThread; // PC->BT moniter thread
static pthread_t rxThread; // BT->PC moniter thread
static BOOL fExit = FALSE;

//////////////////////////////////////////////////////////////

void *bt_tx_monitor(void *ptr);
void *bt_rx_monitor(void *ptr);


static int uart_speed(int s)
{
    switch (s) {
    case 9600:
         return B9600;
    case 19200:
         return B19200;
    case 38400:
         return B38400;
    case 57600:
         return B57600;
    case 115200:
         return B115200;
    case 230400:
         return B230400;
    case 460800:
         return B460800;
    case 500000:
         return B500000;
    case 576000:
         return B576000;
    case 921600:
         return B921600;
    default:
         return B57600;
    }
}

static int init_uart2pc(int port, int speed)
{
    struct termios ti;
    int fd;
    int baudenum;
    char dev[20] = {0};
    
    sprintf(dev, "/dev/ttyMT%d", port);
    fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        ERR("Can't open serial port\n");
        return -1;
    }
    
    tcflush(fd, TCIOFLUSH);
	  
    if (tcgetattr(fd, &ti) < 0) {
        ERR("Can't get UART port setting\n");
        close(fd);
        return -1;
    }
    
    cfmakeraw(&ti);
    
    ti.c_cflag &= ~CSIZE;
    ti.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
    ti.c_oflag &= ~OPOST;
    ti.c_cflag |= CS8;
    
    ti.c_cflag &= ~PARENB;
    ti.c_iflag &= ~INPCK;
    
    ti.c_cflag &= ~CSTOPB;
    /* Set baudrate */
    baudenum = uart_speed(speed);
    if ((baudenum == B57600) && (speed != 57600)) {
        ERR("Serial port baudrate not supported!\n");
        close(fd);
        return -1;
    }
    
    cfsetospeed(&ti, baudenum);
    cfsetispeed(&ti, baudenum);
    
    if (tcsetattr(fd, TCSANOW, &ti) < 0) {
        ERR("Can't set UART port setting\n");
        close(fd);
        return -1;
    }
    
    tcflush(fd, TCIOFLUSH);
    
    return fd;
}


static
int write_data_to_pc(int fd, unsigned char *buffer, unsigned long len)
{
    int bytesWritten = 0;
    int bytesToWrite = len;
    
    if (fd < 0) 
        return -1;
    
    // Send len bytes data in buffer
    while (bytesToWrite > 0){
        bytesWritten = write(fd, buffer, bytesToWrite);
        if (bytesWritten < 0){
            if(errno == EINTR || errno == EAGAIN)
                continue;
            else
                return -1;
        }
        bytesToWrite -= bytesWritten;
        buffer += bytesWritten;
    }

    return 0;
}
int send(unsigned char *buffer, int len){
	ERR("BT803_send\n");
	if(uart_fd >-1)
	{
		int t;
		ERR("BT803_send len = %d\n",len);
		t = write(uart_fd, buffer, len);
		ERR("BT803_send ret = %d\n",t);
		return t;
	}
	else return uart_fd;
	//return write_data_to_pc(uart_fd, buffer, len);
}
static
int read_data_from_pc(int fd, unsigned char *buffer, unsigned long len)
{
    int bytesRead = 0;
    int bytesToRead = len;
    
    int ret = 0;
    struct timeval tv;
    fd_set readfd;
    
    tv.tv_sec = 5;  //SECOND
    tv.tv_usec = 0;   //USECOND
    FD_ZERO(&readfd);
    
    if (fd < 0) 
        return -1;
    
    // Hope to receive len bytes
    while (bytesToRead > 0){
    
        FD_SET(fd, &readfd);
        ret = select(fd + 1, &readfd, NULL, NULL, &tv);
        
        if (ret > 0){
            bytesRead = read(fd, buffer, bytesToRead);
            if (bytesRead < 0){
                if(errno == EINTR || errno == EAGAIN)
                    continue;
                else
                    return -1;
            }
            else{
                bytesToRead -= bytesRead;
                buffer += bytesRead;
            }
        }
        else if (ret == 0){
            return -1; // read commport timeout 5000ms
        }
        else if ((ret == -1) && (errno == EINTR)){
            continue;
        }
        else{
            return -1;
        }
    }

    return 0;
}

static BOOL RELAYER_write(
    int  fd,
    unsigned char *peer_buf, 
    int  peer_len)
{
    
    if (peer_buf == NULL){
        ERR("NULL write buffer\n");
        return FALSE;
    }
    
    if ((peer_buf[0] != 0x04) && (peer_buf[0] != 0x02) && (peer_buf[0] != 0x03)){
        ERR("Invalid packet type 0x%02x to PC\n", peer_buf[0]);
        return FALSE;    
    }
    
    if (write_data_to_pc(fd, peer_buf, peer_len) < 0){
        ERR("Send packet to PC failed\n");
        return FALSE;
    }
    
    return TRUE;
}

static BOOL RELAYER_read(
    int  fd,
    unsigned char *peer_buf, 
    int  peer_len,
    int *piResultLen)
{
    
    UCHAR ucHeader = 0;
    int iLen = 0, pkt_len = 0, count = 0;
    
    if (peer_buf == NULL){
        ERR("NULL read buffer\n");
        return FALSE;
    }

LOOP:
    if(read_data_from_pc(fd, &ucHeader, sizeof(ucHeader)) < 0){
        count ++;
        if (count < 3)
            goto LOOP;
        else
            return FALSE;
    }
    
    peer_buf[0] = ucHeader;
    iLen ++;
    
    switch (ucHeader)
    {
        case 0x01:
            // HCI command
            if(read_data_from_pc(fd, &peer_buf[1], 3) < 0){
                ERR("Receive packet from PC failed\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += 3;
            pkt_len = (int)peer_buf[3];
            if((iLen + pkt_len) > peer_len){
                ERR("Tx buffer overflow! Large packet from PC: len %d\n", iLen + pkt_len);
                *piResultLen = iLen;
                return FALSE;
            }
            
            if(read_data_from_pc(fd, &peer_buf[4], pkt_len) < 0){
                ERR("Receive packet from PC failed\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += pkt_len;
            *piResultLen = iLen;
            break;
                
        case 0x02:
            // ACL data
            if(read_data_from_pc(fd, &peer_buf[1], 4) < 0){
                ERR("Receive packet from PC failed\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += 4;
            pkt_len = (((int)peer_buf[4]) << 8);
            pkt_len += peer_buf[3];//little endian
            if((iLen + pkt_len) > peer_len){
                ERR("Tx buffer overflow! Large packet from PC: len %d\n", iLen + pkt_len);
                *piResultLen = iLen;
                return FALSE;
            }
            
            if(read_data_from_pc(fd, &peer_buf[5], pkt_len) < 0){
                ERR("Receive packet from PC failed\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += pkt_len;
            *piResultLen = iLen;
            break;
            
        case 0x03:
            // SCO data
            if(read_data_from_pc(fd, &peer_buf[1], 3) < 0){
                ERR("Receive packet from PC failed\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += 3;
            pkt_len = (int)peer_buf[3];
            if((iLen + pkt_len) > peer_len){
                ERR("Tx buffer overflow! Large packet from PC: len %d\n", iLen + pkt_len);
                *piResultLen = iLen;
                return FALSE;
            }
            
            if(read_data_from_pc(fd, &peer_buf[4], pkt_len) < 0){
                ERR("Receive packet from PC failed\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += pkt_len;
            *piResultLen = iLen;
            break;
            
        default:
            // Filter PC garbage data
            ERR("Invalid packet type 0x%02x from PC\n", ucHeader);
            return FALSE;
    }
    
    return TRUE;
}


BOOL RELAYER_start(int serial_port, int serial_speed)
{

    if (EM_BT_init()){
        DBG("BT device power on success\n");
    }
    else {
        ERR("BT device power on failed\n");
        return FALSE;
    }
    
    uart_fd = init_uart2pc(serial_port, serial_speed);
    if (uart_fd < 0){
        ERR("Initialize serial port to PC failed\n");
        EM_BT_deinit();
        return FALSE;
    }
    
    /* Create Tx monitor thread */
    pthread_create(&txThread, NULL, bt_tx_monitor, (void*)NULL);
    /* Create RX monitor thread */
    pthread_create(&rxThread, NULL, bt_rx_monitor, (void*)NULL);
    
    DBG("BT Relayer mode start\n");
    fExit = FALSE;
    
    return TRUE;
}
BOOL RELAYER_open(int serial_port, int serial_speed)
{
	RELAYER_close();
    uart_fd = init_uart2pc(serial_port, serial_speed);
    if (uart_fd < 0){
        ERR("Initialize serial port to PC failed\n");
        mOpen = 0;
        return FALSE;
    }
    mOpen = 1;
    return TRUE;
}

void RELAYER_close()
{
	if(uart_fd >-1){
		mOpen = 0;
		sleep(2);//等待线程退出
		close(uart_fd);
	}
    uart_fd = -1;
}

void RELAYER_exit()
{
    fExit = TRUE;
    /* Wait until thread exist */
    pthread_join(txThread, NULL);
    pthread_join(rxThread, NULL);
    
    close(uart_fd);
    uart_fd = -1;
    
    EM_BT_deinit();
}


void *bt_tx_monitor(void *ptr)
{
    UCHAR ucTxBuf[512];
    int iPktLen, i;

    while (!fExit){
        memset(ucTxBuf, 0, sizeof(ucTxBuf));
        iPktLen = 0;

        // Receive HCI packet from PC
        if(RELAYER_read(uart_fd, ucTxBuf, sizeof(ucTxBuf), &iPktLen)){
            // Dump Tx packet
            DBG("Receive packet from PC:\n");
            for (i = 0; i < iPktLen; i++) {
                DBG("%02x\n", ucTxBuf[i]);
            }

            // Send the packet to BT chip
            if(EM_BT_write(ucTxBuf, iPktLen)){
                sched_yield();
            	  continue;
            }
            else{
                ERR("Send packet to BT chip failed\n");
                fExit = TRUE;
            }
        }
        else{
            if (iPktLen == 0)
                continue;
            else
                fExit = TRUE;
        }
    }
    return 0;
}

void *bt_rx_monitor(void *ptr)
{
    UCHAR ucRxBuf[512];
    int iPktLen, i;

    while (!fExit){
        memset(ucRxBuf, 0, sizeof(ucRxBuf));
        int iPktLen = 0;

        // Receive HCI packet from BT chip
        if(EM_BT_read(ucRxBuf, sizeof(ucRxBuf), &iPktLen)){
            // Dump Rx packet
            DBG("Send packet to PC:\n");
            for (i = 0; i < iPktLen; i++) {
                DBG("%02x\n", ucRxBuf[i]);
            }

            // Send the packet to PC
            if(RELAYER_write(uart_fd, ucRxBuf, iPktLen)){
                sched_yield();
                continue;
            }
            else{
                fExit = TRUE;
            }
        }
        else{
            if (iPktLen == 0)
                continue;
            else{
                ERR("Receive packet from BT chip failed\n");
                fExit = TRUE;
            }
        }
    }
    return 0;
}
