
#include "emg.h"
//#define _PRINT_INFO_

#ifdef _PRINT_INFO_
  #define DEBUGLN(X) Serial.println(X)
  #define DEBUG(X) Serial.print(X)
#else
  #define DEBUGLN(X)
  #define DEBUG(X)
#endif

#include <BLEDevice.h>
#include <BLEScan.h>
#include "emg.h"

static bool isConnected = false;
const int LED_BUILTIN = 2;
const int LED_ON = LOW;
const int LED_OFF = HIGH;
volatile int state = LED_OFF;
QueueHandle_t dataQueue = nullptr;



static BLEUUID serviceUUID("e54eeef0-1980-11ea-836a-2e728ce88125");
static BLEUUID DATA_CHARACTERISTIC_UUID("e54eeef1-1980-11ea-836a-2e728ce88125");
static BLEUUID CMD_CHARACTERISTIC_UUID("e54e0002-1980-11ea-836a-2e728ce88125");

static BLEScan* pBLEScan;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pCmdCharacteristic;
static BLEAdvertisedDevice myDevice;
static float batteryLevel = 0;


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{

String mac;
public:
MyAdvertisedDeviceCallbacks(String mac){
  this->mac = mac;
}

 /**
   * Called for each advertising UNIor BLE module.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) 
  {
    #ifdef _PRINT_INFO_
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    #endif
    
    // We have found a device, let us now see if it contains the service we are looking for.
  if (advertisedDevice.getAddress().equals(BLEAddress(mac.c_str())))
   {
     DEBUGLN("Device found...");
     BLEDevice::getScan()->stop();
     myDevice = advertisedDevice;
    // pBLEScan->stop();
   } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


class MyClientCallback : public BLEClientCallbacks 
{
  void onConnect(BLEClient* pclient) {}

  void onDisconnect(BLEClient* pclient) 
  {
    isConnected = false;
    DEBUGLN("onDisconnect");
  }
};


static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) 
{
    float *pv = (float*) pData; 
    pv++; // skip packet time
    float bat = *pv; // battery level, %
    batteryLevel = bat;
    pv++; 
    int cnt = length / sizeof(float);
    for (int i = 2 ; i < cnt; i++) 
    {
      if(uxQueueSpacesAvailable(dataQueue) == 0){
        DEBUGLN("Code is slow, some data is deleted!!!!");
        float tmp;
        xQueueReceive(dataQueue, &tmp, 0);
      }
      xQueueSend(dataQueue, pv, portMAX_DELAY);
      pv++;
    }
}

// connect to UNIor BLE module
bool connectToServer() 
{
    DEBUG("Forming a connection to ");
    DEBUGLN(myDevice.getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    DEBUG(" - Created client");
    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to UNIor BLE module.
    pClient->connect(&myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    DEBUGLN(" - Connected to module");
    // Obtain a reference to the service we are after in the UNIor BLE module.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) 
    {
      DEBUG("Failed to find our service UUID: ");
      DEBUGLN(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    
    DEBUGLN(" - Found our service");
    // Obtain a reference to the characteristic in the service of the UNIor BLE module.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(DATA_CHARACTERISTIC_UUID);
    
    if (pRemoteCharacteristic == nullptr) 
    {
      DEBUG("Failed to find data characteristic UUID: ");
      DEBUGLN(DATA_CHARACTERISTIC_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    DEBUGLN(" - Found data characteristic");
    // Obtain a reference to the characteristic in the service of the UNIor BLE module.
    pCmdCharacteristic = pRemoteService->getCharacteristic(CMD_CHARACTERISTIC_UUID);
    
    if (pCmdCharacteristic == nullptr) 
    {
      
      DEBUG("Failed to find cmd characteristic UUID: ");
      DEBUGLN(CMD_CHARACTERISTIC_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

    DEBUGLN(" - Found cmd characteristic");
    if (pRemoteCharacteristic->canNotify()) 
    {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      DEBUG(" - Notify");
    }

    // send start command to UNIor BLE module
    if (pCmdCharacteristic->canWrite()) 
    {
      uint8_t cmd[6] = {'S', 'T', 'A', 'R', 'T', 0};
      pCmdCharacteristic->writeValue(cmd, 6);
      DEBUGLN(" - start");
    }

    isConnected = true;
}



void EMG::init(String mac){
  if(isConnected){
    return;
  }
  if(dataQueue == nullptr){
    dataQueue = xQueueCreate(100, sizeof(float));
  }

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(mac));
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->start(0, false);
  // pBLEScan->stop
  
  Serial.println(myDevice.getName().c_str());
  
  connectToServer();

}

float EMG::getBatteryLevel(){
  return batteryLevel;
}

float EMG::readValue(){
  float ret;
  xQueueReceive(dataQueue, &ret, portMAX_DELAY);
  return ret;
}
