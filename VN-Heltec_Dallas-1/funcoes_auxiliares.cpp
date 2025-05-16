#include "LoRaWan_APP.h"
#include "virtual_lorawan_callbacks.h"
#include "funcoes_auxiliares.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

void printSession(VirtualNodeSession sess){
  Serial.println("***************************************************************************************");
  Serial.println("Valores armazenados em uma sessao de VN:");

  Serial.printf("devAddr = 0x%08X\n", sess.devAddr);
  
  Serial.printf("fCntUp: %d \t fCntDown: %d\n", sess.fCntUp, sess.fCntDown);
  Serial.println("***************************************************************************************\n");
}

void printFullSession(VirtualNodeSession sess){
  Serial.println("***************************************************************************************");
  Serial.println("Valores armazenados em uma sessao de VN:");

  Serial.printf("devAddr = 0x%08X\n", sess.devAddr);

  Serial.print("nwkSKey: ");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", sess.nwkSKey[i]);
    if (i < 7) Serial.printf(", ");
  }
  Serial.println();

  Serial.print("appSKey: ");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", sess.appSKey[i]);
    if (i < 7) Serial.printf(", ");
  }
  Serial.println();

  
  Serial.printf("fCntUp: %d \t fCntDown: %d\n", sess.fCntUp, sess.fCntDown);
  Serial.println("***************************************************************************************\n");
}

void printCurrentSession(void){
  Serial.println("***************************************************************************************");
  Serial.println("Valores atuais: ");
  Serial.printf("devAddr = 0x%08X\n", devAddr);

  MibRequestConfirm_t mibReq;

  // Get uplink counter
  mibReq.Type = MIB_UPLINK_COUNTER;
  LoRaMacMibGetRequestConfirm(&mibReq);
  Serial.printf("FCntUp: %d\t", mibReq.Param.UpLinkCounter);

  // Get downlink counter
  mibReq.Type = MIB_DOWNLINK_COUNTER;
  LoRaMacMibGetRequestConfirm(&mibReq);
  Serial.printf("FCntDown: %d\n", mibReq.Param.DownLinkCounter);

  Serial.println("***************************************************************************************\n");
  
}

void printFullCurrentSession(void){
  Serial.println("***************************************************************************************");
  Serial.println("Valores atuais: ");
  Serial.printf("devAddr = 0x%08X\n", devAddr);

  Serial.print("nwkSKey: ");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", nwkSKey[i]);
    if (i < 7) Serial.printf(", ");
  }
  Serial.println();

  Serial.print("appSKey: ");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", appSKey[i]);
    if (i < 7) Serial.printf(", ");
  }
  Serial.println();

  MibRequestConfirm_t mibReq;

  // Get uplink counter
  mibReq.Type = MIB_UPLINK_COUNTER;
  LoRaMacMibGetRequestConfirm(&mibReq);
  Serial.printf("FCntUp: %d\n", mibReq.Param.UpLinkCounter);

  // Get downlink counter
  mibReq.Type = MIB_DOWNLINK_COUNTER;
  LoRaMacMibGetRequestConfirm(&mibReq);
  Serial.printf("FCntDown: %d\n", mibReq.Param.DownLinkCounter);

  Serial.println("***************************************************************************************\n");
  
}

bool saveSession(VirtualNodeSession& sess){

  portENTER_CRITICAL(&restoreMux);

  sess.fCntUp = *GetFCntUp();
  sess.fCntDown = *GetFCntDown();

  memcpy(sess.appSKey, GetLoRaMacAppSKey(), sizeof(sess.appSKey));
  memcpy(sess.nwkSKey, GetLoRaMacNwkSKey(), sizeof(sess.nwkSKey));
  sess.devAddr = *GetLoRaMacDevAddr();

  portEXIT_CRITICAL(&restoreMux);
  return true;

/*
  portENTER_CRITICAL(&restoreMux);
  MibRequestConfirm_t mibReq;

  // Get uplink counter
  mibReq.Type = MIB_UPLINK_COUNTER;
  LoRaMacMibGetRequestConfirm(&mibReq);
  sess.fCntUp = mibReq.Param.UpLinkCounter;

  // Get downlink counter
  mibReq.Type = MIB_DOWNLINK_COUNTER;
  LoRaMacMibGetRequestConfirm(&mibReq);
  sess.fCntDown = mibReq.Param.DownLinkCounter;

  memcpy(sess.appSKey, appSKey, sizeof(sess.appSKey));
  memcpy(sess.nwkSKey, nwkSKey, sizeof(sess.nwkSKey));
  sess.devAddr = devAddr;

  portEXIT_CRITICAL(&restoreMux);

  return true;
  */
}

bool restoreSession(const VirtualNodeSession sess) {
    portENTER_CRITICAL(&restoreMux);

    memcpy(GetLoRaMacNwkSKey(), sess.nwkSKey, 16);
    memcpy(GetLoRaMacAppSKey(), sess.appSKey, 16);
    *GetLoRaMacDevAddr() = sess.devAddr;
    *GetFCntUp() = sess.fCntUp;
    *GetFCntDown() = sess.fCntDown;

    devAddr = sess.devAddr;  // if needed for your logging

    portEXIT_CRITICAL(&restoreMux);
    return true;
}

void copyDevEui(uint8_t * _devEui){
  for(int i=0; i<8; i++)
    devEui[i] = _devEui[i];
}



void printTrueCurrentSession() {
  Serial.println("---- True internal session values (from stack) ----");

  uint8_t* realNwkSKey = GetLoRaMacNwkSKey();
  uint8_t* realAppSKey = GetLoRaMacAppSKey();
  uint32_t* realDevAddr = GetLoRaMacDevAddr();
  uint32_t* fCntUp = GetFCntUp();
  uint32_t* fCntDown = GetFCntDown();

  Serial.printf("DevAddr = 0x%08X\n", *realDevAddr);

  Serial.print("NwkSKey: ");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", realNwkSKey[i]);
    if (i < 7) Serial.print(", ");
  }
  Serial.println();

  Serial.print("AppSKey: ");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", realAppSKey[i]);
    if (i < 7) Serial.print(", ");
  }
  Serial.println();

  Serial.printf("FCntUp: %lu\tFCntDown: %lu\n", *fCntUp, *fCntDown);
  Serial.println("----------------------------------------------\n");
}
