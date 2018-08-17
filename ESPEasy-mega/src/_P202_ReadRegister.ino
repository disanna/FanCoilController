//#######################################################################################################
//######################## Plugin 202: Read register ################################
//#######################################################################################################

// Original work by Diego Sanna

// Plugin Description
// This Plugin reads register from memory

#include <SPI.h>
#include <esp8266_peri.h>
#include <ets_sys.h>

#define PLUGIN_202
#define PLUGIN_ID_202 202
#define PLUGIN_NAME_202 "Leggi/scrivi valore registro CPU"
#define PLUGIN_VALUENAME1_202 "Indirizzo registro"
#define PLUGIN_VALUENAME2_202 "Valore registro"

uint16_t reg = 0;
uint32_t valore = 0;

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
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_202));
        break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
        
        addFormNumericBox(string, F("Indirizzo registro (decimale): "), F("plugin_202_registro"), reg, 0, 4095);
        addFormNumericBox(string, F("Valore (decimale): "), F("plugin_202_valore"), valore, 0, 0xffffffff);
        addFormNote(string, "Indirizzo registro (HEX): " + String(reg, 16));
        success = true;
        break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
        reg = getFormItemInt(F("plugin_202_registro"));
        valore = getFormItemInt(F("plugin_202_valore"));
        success = true;
        break;
    }

    case PLUGIN_INIT:
    {
        success = true;
        break;
    }

    case PLUGIN_WRITE:
    {
        String lowerString = string;
        lowerString.toLowerCase();
        String command = parseString(lowerString, 1);
        String param1 = parseString(lowerString, 2);
        String param2 = parseString(lowerString, 3);

        if (command == F("registro"))
        {  
            if (param1 != "") {
                reg = (uint16_t) param1.toInt();
                if ((reg >= 0) && (reg < 4096)) {
                    if(param2 != "") {
                        valore = (uint32_t) param2.toInt();
                        if((valore >= 0) && (valore <= 0xffffffff)) {
                            ESP8266_REG(0xfff & reg) = valore;
                            success = true;
                        } else {
                            success = false;
                        }
                    } else {
                        success = true;
                    }
                } else {
                    reg = 0;
                    success = false;
                }
            }     
        }
        break;
    }

    case PLUGIN_READ:
    {
        valore = ESP8266_REG(0xfff & reg);

        UserVar[event->BaseVarIndex] = reg;
        UserVar[event->BaseVarIndex+1] = valore;

        String log = F("Registro");
        log += reg;
        log += F(": ");
        
        for(int i=31;i>=0;i--) {
            if((1<<i)&valore) 
                log += '1';
            else
                log += '0';
            if(i==8||i==16||i==24) 
                log += '|';
            if(i==28||i==20||i==12||i==4)
                log += '.';
        }
        addLog(LOG_LEVEL_INFO, log);

        success = true;
        break;
    }
    }
    return success;
}
