#include "LoRaWan_APP.h"
#include "virtual_lorawan_callbacks.h"
#include <queue>


std::queue<int> virtualNodesReadyToSend;

/**********************************************************
 * Callback: apenas insere que tem coisa para transmitir  *
 * na fila.                                               *
 *                                                        *
 * NAO pode iniciar o proximo timer: depende do indice de *
 * satisfacao do gurgame                                  *
 *                                                        *
 * Start do proximo timer apos fim das RX window          *
 * TimerStart, timerset is in virtual_lorawan.cycle()     *
 **********************************************************/
void VN0_nextTXTimerCallback(void){
  //Serial.println("****************************************************************************************");
  //Serial.println("Sou o VN 0 e fiz virtualNodesReadyToSend.push(0)");
  virtualNodesReadyToSend.push(0);
  //Serial.println("****************************************************************************************");
}

void VN1_nextTXTimerCallback(void){
  //Serial.println("****************************************************************************************");
  //Serial.println("Sou o VN 1 e fiz virtualNodesReadyToSend.push(1)");
  virtualNodesReadyToSend.push(1);
  //Serial.println("****************************************************************************************");
}

void VN2_nextTXTimerCallback(void){
  //Serial.println("****************************************************************************************");
  //Serial.println("Sou o VN 2 e fiz virtualNodesReadyToSend.push(2)");
  virtualNodesReadyToSend.push(2);
  //Serial.println("****************************************************************************************");
}
