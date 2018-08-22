//#######################################################################################################
//######################## Plugin 202: Prova I2C ################################
//#######################################################################################################

// Original work by Diego Sanna

// Plugin Description
// Questo plugin serve per controllare il ventilconvettore Olimpia Splendid utilizzando un modulo Arduino Nano
// che si interfaccia tramite la linea SPI con il tastierino del ventilconvettore e tramite la linea I2C con il modulo ES8266

#include <esp8266_peri.h>
#include <ets_sys.h>

#define PLUGIN_202
#define PLUGIN_ID_202 202
#define PLUGIN_NAME_202 "Prova I2C"
#define PLUGIN_VALUENAME1_202 "valore_ricevuto"

#define WIRE_ADDRESS 1

boolean Plugin_202(byte function, struct EventStruct *event, String &string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number = PLUGIN_ID_202;
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
      string = F(PLUGIN_NAME_202);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_202));
       break;
    }

    case PLUGIN_INIT:
    {
      Wire.begin(WIRE_ADDRESS);
      Wire.setClockStretchLimit(40000);

      success = true;
      break;
    }

    case PLUGIN_READ:
    {
        String log = F("Fancoil - Read - ");
        
        Wire.beginTransmission(WIRE_ADDRESS);
        Wire.write((uint8_t) 123);
        Wire.endTransmission();
        delay(1);

        byte count = Wire.requestFrom(WIRE_ADDRESS, 1);
        log += F("conut:");
        log += count;

        delay(100);        
        uint8_t v = Wire.read();

        log += F(" value (bin):");
        log = log + String(v, BIN);
        addLog(LOG_LEVEL_DEBUG, log);
        log += F(" value (dec):");
        log += String(v,DEC);
        addLog(LOG_LEVEL_DEBUG, log);
        UserVar[event->BaseVarIndex] = v;

        delay(100);
        Wire.beginTransmission(WIRE_ADDRESS);
        Wire.write((uint8_t) v);
        Wire.endTransmission();
        success = true;
      break;
    }

    case PLUGIN_WRITE:
    {
      success = true;
      break;
    }
  }
  return success;
}
