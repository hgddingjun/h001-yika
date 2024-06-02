
#ifndef __EM_BT_H__
#define __EM_BT_H__

#include <stdbool.h>
#ifndef BOOL
#define BOOL  bool
#endif

typedef enum {
  BT_CHIP_ID_MT6611 = 0,
  BT_CHIP_ID_MT6612,
  BT_CHIP_ID_MT6616,
  BT_CHIP_ID_MT6620,
  BT_CHIP_ID_MT6622,
  BT_CHIP_ID_MT6626,
  BT_CHIP_ID_MT6628
} BT_CHIP_ID;

typedef enum {
  BT_HW_ECO_UNKNOWN = 0,
  BT_HW_ECO_E1,
  BT_HW_ECO_E2,
  BT_HW_ECO_E3,
  BT_HW_ECO_E4,
  BT_HW_ECO_E5,
  BT_HW_ECO_E6,
  BT_HW_ECO_E7
} BT_HW_ECO;


#ifdef __cplusplus
extern "C"
{
#endif

int send(unsigned char *buffer, int len);
BOOL RELAYER_start(int serial_port, int serial_speed);
void RELAYER_exit(void);    
BOOL RELAYER_open(int serial_port, int serial_speed);
void RELAYER_close();

BOOL EM_BT_init(void);
void EM_BT_deinit(void);
BOOL EM_BT_write(unsigned char *peer_buf, int peer_len);
BOOL EM_BT_read(unsigned char *peer_buf, int peer_len, int *piResultLen);
void EM_BT_polling_start(void);
void EM_BT_polling_stop(void);

void EM_BT_getChipInfo(BT_CHIP_ID *chip_id, BT_HW_ECO *eco_num);
void EM_BT_getPatchInfo(char *patch_id, unsigned long *patch_len);
#ifdef __cplusplus
}
#endif

#endif
