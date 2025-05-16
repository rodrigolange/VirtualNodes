#ifndef VIRTUAL_LORAWAN_CALLBACKS_H
#define VIRTUAL_LORAWAN_CALLBACKS_H

#include <queue>

extern std::queue<int> virtualNodesReadyToSend;

/********************************************
 * Virtual node VN0                         *
 ********************************************/
void VN0_nextTXTimerCallback(void);
extern TimerEvent_t VN0_txNextPacketTmr;
extern uint32_t VN0_dutyCycle;

/********************************************
 * Virtual node VN1                         *
 ********************************************/
void VN1_nextTXTimerCallback(void);
extern TimerEvent_t VN1_txNextPacketTmr;
extern uint32_t VN1_dutyCycle;

/********************************************
 * Virtual node VN2                         *
 ********************************************/
void VN2_nextTXTimerCallback(void);
extern TimerEvent_t VN2_txNextPacketTmr;
extern uint32_t VN2_dutyCycle;


#endif