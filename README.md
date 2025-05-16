
# Virtual LoRaWAN Nodes on Heltec ESP32 WiFi LoRa 32 V2

This project implements **multiple virtual LoRaWAN nodes (VNs)** on a **single Heltec ESP32 WiFi LoRa 32 v2** board. It uses the **Semtech LoRaWAN stack** as modified by Heltec, running under the **Arduino IDE 2.0**.

## ✅ Overview

- **Platform**: Heltec ESP32 WiFi LoRa 32 v2
- **Framework**: Arduino IDE 2.0
- **LoRaWAN Stack**: Semtech stack with Heltec-specific modifications
- **Virtual Nodes**: Multiple logical LoRaWAN devices simulated on a single physical board

## ✅ Virtual Node Architecture

Each virtual node (VN0, VN1, VN2) maintains:
- Unique `DevEUI`, `AppSKey`, `NwkSKey`, and `DevAddr`
- Separate uplink (`fCntUp`) and downlink (`fCntDown`) counters
- Individual LoRaWAN session context (`VirtualNodeSession` struct)
- Independent callbacks and timer instances

## ✅ Session Management

### Saving Session (`saveSession()`)
- Frame counters and keys are accessed **directly from internal Semtech variables**, not MIBs:
  ```cpp
  sess.fCntUp    = *GetFCntUp();
  sess.fCntDown  = *GetFCntDown();
  memcpy(sess.nwkSKey, GetLoRaMacNwkSKey(), 16);
  memcpy(sess.appSKey, GetLoRaMacAppSKey(), 16);
  sess.devAddr   = *GetLoRaMacDevAddr();
  ```

### Restoring Session (`restoreSession()`)
- Internal state is restored before each transmission using custom setters:
  ```cpp
  memcpy(GetLoRaMacNwkSKey(), sess.nwkSKey, 16);
  memcpy(GetLoRaMacAppSKey(), sess.appSKey, 16);
  *GetLoRaMacDevAddr() = sess.devAddr;
  *GetFCntUp() = sess.fCntUp;
  *GetFCntDown() = sess.fCntDown;
  ```

## File Modifications

### ✅ Heltec Library Modifications

#### In `Arduino/libraries/Heltec_ESP32_Dev-Boards/src/LoRaWan_APP.CPP`

Several modifications.

### ✅ Semtech Stack Modifications

**Arduino/libraries/Heltec_ESP32_Dev-Boards/src/loramac**

#### In `LoRaMac.c`
Custom getter functions were added to expose internal static variables:
```c
uint8_t* GetLoRaMacNwkSKey(void) { return LoRaMacNwkSKey; }
uint8_t* GetLoRaMacAppSKey(void) { return LoRaMacAppSKey; }
uint32_t* GetLoRaMacDevAddr(void) { return &LoRaMacDevAddr; }
uint32_t* GetFCntUp(void) { return &UpLinkCounter; }
uint32_t* GetFCntDown(void) { return &DownLinkCounter; }
```

#### In `LoRaMac.h`
Added prototypes for the above functions.

## ✅ Runtime Switching

- Managed by a queue (`std::queue<int> virtualNodesReadyToSend`)
- Each VN is scheduled to send based on timer triggers
- Session context is saved and restored before/after transmission
- `txInProgress` flag ensures RX windows are honored, reset at the end of `McpsConfirm()`

