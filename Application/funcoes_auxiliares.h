#ifndef FUNCOES_AUXILIARES_H
#define FUNCOES_AUXILIARES_H

extern portMUX_TYPE restoreMux;

struct VirtualNodeSession {
  uint32_t devAddr    = 0;
  uint8_t nwkSKey[16] = {0};
  uint8_t appSKey[16] = {0};
  uint32_t fCntUp     = 0;
  uint32_t fCntDown   = 0;
  bool adrEnabled     = true;
  bool isJoined       = false;
  bool isStored       = false;
};

void printSession(VirtualNodeSession sess);
void printFullSession(VirtualNodeSession sess);

void printCurrentSession(void);
void printFullCurrentSession(void);

void printTrueCurrentSession();

bool saveSession(VirtualNodeSession& sess);

bool restoreSession(const VirtualNodeSession sess);

bool saveFrameCount(VirtualNodeSession& sess);
void copyDevEui(uint8_t * _devEui);

#endif