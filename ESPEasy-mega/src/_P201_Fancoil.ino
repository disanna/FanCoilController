//#######################################################################################################
//######################## Plugin 201: Fancoil Olimpia Splendid SL ################################
//#######################################################################################################

// Original work by Diego Sanna

// Plugin Description
// Questo plugin serve per controllare il ventilconvettore Olimpia Splendid utilizzando un modulo Arduino Nano
// che si interfaccia tramite la linea SPI con il tastierino del ventilconvettore e tramite la linea I2C con il modulo ES8266

#include <esp8266_peri.h>
#include <ets_sys.h>
#include <wire.h>

#define PLUGIN_201
#define PLUGIN_ID_201 201
#define PLUGIN_NAME_201 "Controllo Fancoil Olimpia SL"
#define PLUGIN_VALUENAME1_201 "Temperatura"
#define PLUGIN_VALUENAME2_201 "Modalità"

#define POWER_OFF 0
#define MODE_AUTO 1
#define MODE_MIN 2
#define MODE_MAX 3
#define MODE_NIGHT 4
#define WIRE_ADDRESS 1
#define I2C_MSG_IN_SIZE    4
#define GET_TEMP 0
#define GET_MODE 1
#define SET_TEMP 2
#define SET_MODE 3

volatile uint8_t sendBuffer[I2C_MSG_OUT_SIZE];
float fancoilTemp;
uint8_t fanocoilMode;

boolean Plugin_201(byte function, struct EventStruct *event, String &string)
{
  boolean success = false;

  switch (function)
  {
  case PLUGIN_DEVICE_ADD:
  {
    Device[++deviceCount].Number = PLUGIN_ID_201;
    Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
    Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
    Device[deviceCount].Ports = 0;
    Device[deviceCount].PullUpOption = false;
    Device[deviceCount].InverseLogicOption = false;
    Device[deviceCount].FormulaOption = true;
    Device[deviceCount].ValueCount = 2;
    Device[deviceCount].SendDataOption = true;
    Device[deviceCount].TimerOption = true;
    Device[deviceCount].GlobalSyncOption = true;
    break;
  }

  case PLUGIN_GET_DEVICENAME:
  {
    string = F(PLUGIN_NAME_201);
    break;
  }

  case PLUGIN_GET_DEVICEVALUENAMES:
  {
    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_201));
    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_201));
    break;
  }

  case PLUGIN_INIT:
  {
    Wire.begin();
    success = true;
    break;
  }

  case PLUGIN_READ:
  {
    if(getFancoilTemp()) {  
      UserVar[event->BaseVarIndex] = fancoilTemp; //legge valore set point impostato sul fancoil
      success = true;
    } else {
      success = false;
    }
    if(getFancoilMode()) {  
      UserVar[event->BaseVarIndex+1] = fanocoilMode; //legge valore set point impostato sul fancoil
      success = true;
    } else {
      success = false;
    }
    break;
  }
  }
  return success;
}

bool void getFancoilTemp() {
  Wire.beginTransmission(WIRE_ADDRESS);
  Wire.write(GET_TEMP);
  Wire.endTransmission(WIRE_ADDRESS);
  Wire.requestFrom(ARDUINO_ADDRESS, 1); 

  if(1 == Wire.available())  {
    fancoilTemp = Wire.read() + 100;
    return true;
  } else {
    return false;
  }
}

bool void getFancoilMode() {
  Wire.beginTransmission(WIRE_ADDRESS);
  Wire.write(GET_MODE);
  Wire.endTransmission(WIRE_ADDRESS);
  Wire.requestFrom(ARDUINO_ADDRESS, 1); 
  
  if(1 == Wire.available())  {
    fanocoilMode = Wire.read();
    return true;
  } else {
    return false;
  }
}