#ifndef DALI_H 
#define DALI_H 

#define BROADCAST_C  0xff
#define BROADCAST_P  0xfe

#define OFF_C        0x00
#define ON_C         0x05
#define QUERY_STATUS 0x90

#define DALI_C_INITIALIZE 0xa5
#define DALI_C_RANDOMIZE  0xa7
#define DALI_C_COMPARE    0xa9
#define DALI_C_WITHDRAW   0xab
#define DALI_C_PING       0xad
#define DALI_C_RESET      0x20

#include <zephyr/kernel.h>

int dali_receive_byte();
void dali_send_data(uint8_t _addr, uint8_t _command);

#endif /* DALI_H */
