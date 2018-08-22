//#######################################################################################################
//######################## Plugin 201: Fancoil Olimpia Splendid SL ################################
//#######################################################################################################

// Original work by Diego Sanna

// Plugin Description
// Questo plugin serve per controllare il ventilconvettore Olimpia Splendid utilizzando un modulo Arduino Nano
// che si interfaccia tramite la linea SPI con il tastierino del ventilconvettore e tramite la linea I2C con il modulo ES8266

#include <esp8266_peri.h>
#include <ets_sys.h>

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
#define MODE_DISCONNECTED 5
#define WIRE_ADDRESS 1
#define I2C_MSG_IN_SIZE    4
#define GET_TEMP 0
#define GET_MODE 1
#define SET_TEMP 2
#define SET_MODE 3
#define INC_TEMP 4
#define DEC_TEMP 5

boolean Plugin_201(byte function, struct EventStruct *event, String &string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number = PLUGIN_ID_201;
      Device[deviceCount].Type = DEVICE_TYPE_I2C;
      Device[deviceCount].VType = SENSOR_TYPE_DUAL;
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
      Wire.begin(WIRE_ADDRESS);
      Wire.setClockStretchLimit(40000); //necessario per evitare errori di funzionamento della comunicazione I2C tra arduino nano e ESP8266
      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      String log = F("Fancoil - Read");
      addLog(LOG_LEVEL_DEBUG, log);

      uint16_t fancoilTemp = getFancoilTemp();
      if(fancoilTemp != 255) {
        UserVar[event->BaseVarIndex] = fancoilTemp; //legge valore set point impostato sul fancoil
        success = true;
      } else {
        success = false;
      }
      log = F("Temperatura= ");
      log += fancoilTemp;
      addLog(LOG_LEVEL_DEBUG, log);

      uint8_t fancoilMode = getFancoilMode();
      if(fancoilMode != 255) {  
        UserVar[event->BaseVarIndex+1] = fancoilMode; //legge valore set point impostato sul fancoil
        success = true;
      } else {
        success = false;
      }
      log = F("Modalità= ");
      log += fancoilMode;
      addLog(LOG_LEVEL_DEBUG, log);

      break;
    }

    case PLUGIN_WRITE:
    {
      String command = parseString(string, 1);

      if (command.equalsIgnoreCase(F("FancoilSetTemp")))
      {
        int param = (parseString(string, 2)).toInt();
        
        String log = F("ricevuto comando FancoilSetTemp");

        delay(1);
        Wire.beginTransmission(WIRE_ADDRESS);
        Wire.write(SET_TEMP);
        I2CWriteTwoBytes(param);
        Wire.endTransmission();

        delay(1);
        Wire.requestFrom(WIRE_ADDRESS,2);
        I2CReadTwoBytes();

        success = true;
      }

      else if(command.equalsIgnoreCase(F("FancoilSetMode"))) 
      {
        int param = (parseString(string, 2)).toInt();
        
        String log = F("ricevuto comando FancoilSetMode");

        delay(1);
        Wire.beginTransmission(WIRE_ADDRESS);
        Wire.write(SET_MODE);
        Wire.write(param);
        Wire.endTransmission();

        delay(1);
        Wire.requestFrom(WIRE_ADDRESS,1);
        Wire.read();

        success = true;
      }

      else if(command.equalsIgnoreCase(F("FancoilIncTemp"))) 
      {
        String log = F("ricevuto comando FancoilIncTemp");

        delay(1);
        Wire.beginTransmission(WIRE_ADDRESS);
        Wire.write(INC_TEMP);
        Wire.endTransmission();

        delay(1);
        Wire.requestFrom(WIRE_ADDRESS,2);
        I2CReadTwoBytes();

        success = true;
      }  
      
      else if(command.equalsIgnoreCase(F("FancoilDecTemp"))) 
      {
        String log = F("ricevuto comando FancoilDecTemp");

        delay(1);
        Wire.beginTransmission(WIRE_ADDRESS);
        Wire.write(DEC_TEMP);
        Wire.endTransmission();

        delay(1);
        Wire.requestFrom(WIRE_ADDRESS,2);
        I2CReadTwoBytes();

        success = true;
      } 

      else {
        success = false;
      } 

      break;
      
    }
  }
  return success;
}

uint16_t getFancoilTemp() {
  uint16_t fancoilTemp = 255;

  delay(1);
  Wire.beginTransmission(WIRE_ADDRESS);
  Wire.write(GET_TEMP);
  Wire.endTransmission();
  
  delay(1);
  Wire.requestFrom(WIRE_ADDRESS, 2); 
  
  delay(1);
  fancoilTemp = I2CReadTwoBytes();

  return fancoilTemp;
}

uint8_t getFancoilMode() {
  uint8_t fancoilMode = 255;

  delay(1);
  Wire.beginTransmission(WIRE_ADDRESS);
  Wire.write(GET_MODE);
  Wire.endTransmission();

  delay(1);
  Wire.requestFrom(WIRE_ADDRESS, 1); 
  
  delay(1);
  fancoilMode = Wire.read();

  return fancoilMode;
}

uint16_t I2CReadTwoBytes() {
  uint16_t result = Wire.read() *256;
  result += Wire.read();
  return result;
}

void I2CWriteTwoBytes(uint16_t value) {
  Wire.write((uint8_t)(value >> 8));
  Wire.write((uint8_t)value);
}
