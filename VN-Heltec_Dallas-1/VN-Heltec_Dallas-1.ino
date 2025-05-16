/********************************************************************************
 * Versao modificada em 07/05/2025                                              *
 * Instalacao nova de:                                                          *
 * IDE Arduino V2                                                               *
 * Heltec Libraries                                                             *
 * Esp32 boards usando                                                          *
 * https://resource.heltec.cn/download/package_heltec_esp32_index.json          *
 *                                                                              *
 * - Copiar LoRaWan_APP .cpp e .h para libraries/heltec.../src                  *
 * - em Arduino2\libraries\OneWire\util\OneWire_direct_gpio.h                   *
 *    -- adicionar no inicio do arquivo                                         *
 * #define GPIO_IS_VALID_GPIO(pin) ((pin) < 40 && (pin) != 20 && (pin) != 24)   *
 *                                                                              *
 * ToDo:                                                                        *
 ********************************************************************************/

/********************************************************************************
 *                             Anotacoes                                        *
 *                                                                              *
 * Para abrir a linha de comando que chama o exception decoder: F1              *
 *                                                                              *
 *                                                                              *
 * Metodos do Semtech stack:                                                    *
 *                                                                              *
 * McpsConfirm()	    Confirms message was sent + RX windows closed             *
 * McpsIndication()	  Informs you of a received downlink                        *
 * MlmeConfirm()	    Confirms MAC-layer operations (like join)                 *
 * MlmeIndication()	  Informs of MAC-layer events (like link loss)              *
 *                                                                              *
 *                                                                              *
 ********************************************************************************/
 
#include "LoRaWan_APP.h"
#include "funcoes_auxiliares.h"
#include "virtual_lorawan_callbacks.h"


#include <DallasTemperature.h>   // by Miles Burton
#include "HT_SSD1306Wire.h"

#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <math.h>

#include <queue>

uint32_t millisTimer = 30000;
uint32_t millisTimer2 = 60000;

bool txInProgressAnt = false;

portMUX_TYPE restoreMux = portMUX_INITIALIZER_UNLOCKED;

bool sessionStored;
int currentVirtualNode;

std::queue<int> virtualNodesJoinQueue;

uint8_t devEuiVN[3][8]     = {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x05},
                              {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01},
                              {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02}
                             };


TimerEvent_t VN0_txNextPacketTmr;
uint32_t VN0_dutyCycle  = 60000;

TimerEvent_t VN1_txNextPacketTmr;
uint32_t VN1_dutyCycle  = 60000;

TimerEvent_t VN2_txNextPacketTmr;
uint32_t VN2_dutyCycle  = 60000;

void (*nextPacketSendCallbacks[3])(void) = {
    VN0_nextTXTimerCallback,
    VN1_nextTXTimerCallback,
    VN2_nextTXTimerCallback
};

// Array of TimerEvent_t pointers
TimerEvent_t* txNextPacketTimers[3] = {
    &VN0_txNextPacketTmr,
    &VN1_txNextPacketTmr,
    &VN2_txNextPacketTmr
};

VirtualNodeSession sessionVN[3];


/*******************************************************************
 * Sensor de temperatura Dallas ds18b20                            *
 *******************************************************************/
#define ONE_WIRE_BUS 13
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temperatura = 25.0;

/*******************************************************************
 * OLED Arguments                                                  *
 *******************************************************************/
#define OLED_UPDATE_INTERVAL 500          //OLED screen refresh interval ms
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

/*************************************************************************
 * Definicoes LoRaWAN                                                    *
 * Chaves, IDs, etc                                                      *
 * DEVEUI: MSB first, em relacao ao servidor chirpstack                  *
 * 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01                        *
 * byte[MSB]: 0x0? = ID aplicacao                                        *
 *                                                                       *
 * byte[1]: 0x20 = campus                                                *
 *          0x10 = casa                                                  *
 *          0x50 = virtual node                                          *
 * byte[0]: 0xXX = ID placa                                              *
 *************************************************************************/
uint32_t appTxDutyCycle = 60000;

/*************************************************************************
 * Placeholders. Necessario devido a forma como o codigo Heltec funciona *
 *************************************************************************/
uint8_t devEui[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x05 };
uint8_t appEui[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
uint8_t appKey[] = { 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01 };

uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*************************************************************************
 * Parametros para a rede LoRaWAN                                        *
 *************************************************************************/
uint32_t license[4] = {0xCE0B4DE0, 0xC56F688B, 0xDA14D311, 0xA4CDFE12};
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_AU915; //ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

/*************************************************************************
 * Dados do WiFi para update via rede                                    *
 *************************************************************************/
//const char *ssid = "Lange-WiFi";
//const char *password = "R0dr1g0L4ng3";
//const char* deviceName = "Casa-Escritorio-2";
//IPAddress ip;

static void prepareTxFrame( uint8_t port ){
  sensors.requestTemperatures();
  temperatura = sensors.getTempCByIndex(0);

  //printValues();

  int16_t int_temp = (int16_t)(temperatura * 100);
  int16_t appTxDutyCycle_temp = (int16_t)(appTxDutyCycle/1000);

  appDataSize = 20;

  appData[0] = int_temp >> 8;
  appData[1] = int_temp & 0xFF;

  appData[2] = 0;
  appData[3] = 0;

  appData[4] = 0;
  appData[5] = 0;

  appData[6] = 0;

  appData[7]  = 'C';
  appData[8]  = 'A';
  appData[9]  = 'S';
  appData[10] = 'A';
  appData[11] = '-';
  appData[12] = 'V';
  appData[13] = 'N';  
  appData[14] = '-';
  appData[15] = '0' + currentVirtualNode; //'2';
  appData[16] = '\0';

  appData[17] = 0; //ip[3];'

  appData[18] = appTxDutyCycle_temp >> 8;
  appData[19] = appTxDutyCycle_temp & 0xFF;

}

//downlink data handle function
void downLinkDataHandle(McpsIndication_t *mcpsIndication){

  //Serial.println("Estou em downLinkDataHandle");
  if (mcpsIndication->BufferSize >= 8) {
    double innerPackedData;
    memcpy(&innerPackedData, mcpsIndication->Buffer, sizeof(double));

    double originalValue;
    memcpy(&originalValue, &innerPackedData, sizeof(double));

    double indiceSatisfacao = static_cast<double>(originalValue);

    Serial.println("****************************************************************************************");
    Serial.print(millis());
    Serial.print(" - ");    
    Serial.println("Indice de satisfacao: " + String(indiceSatisfacao, 10));

    // cycle para escalonar novo push
    uint32_t novoDT = appTxDutyCycle + random(60000, 120001);
    TimerSetValue(txNextPacketTimers[currentVirtualNode], novoDT);
    TimerStart(txNextPacketTimers[currentVirtualNode]);
    Serial.println(String(millis()) +  " - Proxima transmissao do VN " + String(currentVirtualNode) + " agendada para " + String(millis() + novoDT));

    //Serial.println("Terminei o processo de enviar e receber pacote de VN. currentVirtualNode = " + String(currentVirtualNode));
    saveSession(sessionVN[currentVirtualNode]);
    Serial.println("****************************************************************************************\n");

    deviceState = DEVICE_STATE_SLEEP;
  } else {
    Serial.println("Error: Buffer size too small to unpack a double.");
  }
}

void setup() {
  Serial.begin(115200);

  randomSeed(esp_random());

  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
  sensors.begin();
  sensors.requestTemperatures();
  temperatura = sensors.getTempCByIndex(0);

  display.init();
  display.setFont(ArialMT_Plain_16);
  display.setContrast(255);
 // printValues();

  //virtualNodesJoinQueue.push(0);                   // a 0 nao vai por ser a primeira
  virtualNodesJoinQueue.push(1);
  virtualNodesJoinQueue.push(2);

  for (int i = 0; i < 3; i++) {
    Serial.print("Initializing VN");
    Serial.println(i);
    copyDevEui(devEuiVN[i]);
    saveSession(sessionVN[i]);
    TimerInit(txNextPacketTimers[i], nextPacketSendCallbacks[i]);
  }

  /*******************************************
   * Init first virtual node and start cycle *
   * ToDo: init all virtual nodes here, and  *
   * not in the loop state                   *
   *******************************************/
  currentVirtualNode  = 0;
  copyDevEui(devEuiVN[currentVirtualNode]);
  sessionStored       = false;                                            // precisa para saber se estah ok no Semtech e Heltec app
  restoreSession(sessionVN[currentVirtualNode]);
  copyDevEui(devEuiVN[currentVirtualNode]);
  deviceState         = DEVICE_STATE_INIT;
}

void loop(){
  /*
  if(millis() > millisTimer){                       // I am alive each 5 minutes
    Serial.println(String(millis()) + " - Ping");
    millisTimer = millis() + 120000;
  }
  */
  /**********************************************************************************
   * A maquina de estados eh necessaria porque diversas chamadas sao assincronas    *
   *                                                                                *
   * Exemplo: LoRaWAN.send() dispara o envio em background e retorna imediatamente  *
   *          com a maquina de estados, ele aguarda ate o envio estar completo, mas *
   *          o scheduling do proximo envio jah eh realizado                        *
   **********************************************************************************/
  switch (deviceState) {
    case DEVICE_STATE_INIT:
    {
        LoRaWAN.init(loraWanClass, loraWanRegion);
        LoRaWAN.setDefaultDR(3);
        break;
    }
    case DEVICE_STATE_JOIN:
      /************************************************
       * Lembrar de setar o primeiro nodo no setup()  *
       ************************************************/
      LoRaWAN.join();
      break;

    case DEVICE_STATE_SEND:
    {
      restoreSession(sessionVN[currentVirtualNode]);
      prepareTxFrame(appPort);
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }

    case DEVICE_STATE_CYCLE:
    {
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }

    case DEVICE_STATE_SLEEP:
    {
      /****************************************************************************************
       * txInProgress: nao faz nada, precisa esperar o hardware terminar a comunicacao  atual *
       ****************************************************************************************/
      if (txInProgress) {
        //Serial.println("[INFO] Waiting for TX to finish before continuing...");
        deviceState = DEVICE_STATE_SLEEP;
        LoRaWAN.sleep(loraWanClass);
        break;        
      }else{
       /*************************************************************************************
        * Ok, pode fazer algo como join ou agendar a proxima transmissao de um VN           *
        *                                                                                   *
        * Aqui serah feita toda a logica de selecao de nodo virtual                         *
        *                                                                                   *
        * Inicialmente apenas fazer join, testando a flag                                   *
        *************************************************************************************/
        if(sessionStored && !sessionVN[currentVirtualNode].isStored){                       // Processo de join terminou e as chaves foram atualizadas para o VN
                                                                                            // testa session.sessionStored para evitar fazer em cada loop
          saveSession(sessionVN[currentVirtualNode]);
          sessionVN[currentVirtualNode].isStored = true;                                    // atualiza flag na session temp

          Serial.println("****************************************************************************************");
          Serial.println(String(millis()) + " - Join do VN " + String(currentVirtualNode) + " terminou.");
          uint32_t next = appTxDutyCycle+ random(10000, 90001);
          TimerSetValue(txNextPacketTimers[currentVirtualNode], next);
  	      TimerStart(txNextPacketTimers[currentVirtualNode]);
          Serial.println(String(millis()) + " - proxima transmissao agendada para " + String(next));
          Serial.println("****************************************************************************************\n");

          if(!virtualNodesJoinQueue.empty()){
            currentVirtualNode  = virtualNodesJoinQueue.front();
            virtualNodesJoinQueue.pop();
            sessionStored       = false;                                            // precisa para saber se estah ok no Semtech e Heltec app
            
            restoreSession(sessionVN[currentVirtualNode]);
            sessionVN[currentVirtualNode].isStored    = false;
            devAddr = 0x000000001 + (uint32_t)currentVirtualNode;
            memcpy(devEui, devEuiVN[currentVirtualNode], 8);
            LoRaWAN.init(loraWanClass, loraWanRegion);
            deviceState         = DEVICE_STATE_JOIN;
            break;
          }
        }else if(!virtualNodesReadyToSend.empty()){
          currentVirtualNode  = virtualNodesReadyToSend.front();
          virtualNodesReadyToSend.pop();
          Serial.println("****************************************************************************************");
          Serial.print(String(millis()) + " - Send do VN " + String(currentVirtualNode));

          Serial.printf(" - devAddr = 0x%08X\n", sessionVN[currentVirtualNode].devAddr);
          deviceState = DEVICE_STATE_SEND;
          Serial.println("****************************************************************************************\n");          
          break;
        }else{
          deviceState = DEVICE_STATE_SLEEP;
          LoRaWAN.sleep(loraWanClass);
          break;
        }
      }
      deviceState = DEVICE_STATE_SLEEP;
      //LoRaWAN.sleep(loraWanClass);
      break;
    }

    default:
        deviceState = DEVICE_STATE_INIT;
        break;
  }
}
