//#######################################################################################################
//######################## Plugin 202: Read register ################################
//#######################################################################################################

// Original work by Diego Sanna

// Plugin Description
// This Plugin reads register from memory

#include <SPI.h>
#include <esp8266_peri.h>
#include <ets_sys.h>
#include <Arduino.h>
#include <functional>

typedef std::function<void(uint8_t *data, size_t len)> SpiSlaveDataHandler;
typedef std::function<void(uint32_t status)> SpiSlaveStatusHandler;
typedef std::function<void(void)> SpiSlaveSentHandler;

class SPISlaveClass
{
  protected:
    SpiSlaveDataHandler _data_cb;
    SpiSlaveStatusHandler _status_cb;
    SpiSlaveSentHandler _data_sent_cb;
    SpiSlaveSentHandler _status_sent_cb;
    void _data_rx(uint8_t *data, uint8_t len);
    void _status_rx(uint32_t data);
    void _data_tx(void);
    void _status_tx(void);
    static void _s_data_rx(void *arg, uint8_t *data, uint8_t len);
    static void _s_status_rx(void *arg, uint32_t data);
    static void _s_data_tx(void *arg);
    static void _s_status_tx(void *arg);

  public:
    SPISlaveClass()
        : _data_cb(NULL), _status_cb(NULL), _data_sent_cb(NULL), _status_sent_cb(NULL)
    {
    }
    ~SPISlaveClass() {}
    void begin();
    void setData(uint8_t *data, size_t len);
    void setData(const char *data)
    {
        setData((uint8_t *)data, strlen(data));
    }
    void setStatus(uint32_t status);
    void onData(SpiSlaveDataHandler cb);
    void onDataSent(SpiSlaveSentHandler cb);
    void onStatus(SpiSlaveStatusHandler cb);
    void onStatusSent(SpiSlaveSentHandler cb);
};


SPISlaveClass SPISlave;

#define PLUGIN_203
#define PLUGIN_ID_203 203
#define PLUGIN_NAME_203 "Prova SPI Slave"
#define PLUGIN_VALUENAME1_203 "na"
#define PLUGIN_VALUENAME2_203 "na"

boolean Plugin_203(byte function, struct EventStruct *event, String &string)
{
    boolean success = false;

    switch (function)
    {
    case PLUGIN_DEVICE_ADD:
    {
        Device[++deviceCount].Number = PLUGIN_ID_203;
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
        string = F(PLUGIN_NAME_203);
        break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_203));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_203));
        break;
    }

    case PLUGIN_INIT:
    {
        Serial.begin(115200);
        Serial.setDebugOutput(true);

        // data has been received from the master. Beware that len is always 32
        // and the buffer is autofilled with zeroes if data is less than 32 bytes long
        // It's up to the user to implement protocol for handling data length
        SPISlave.onData([](uint8_t *data, size_t len) {
            String message = String(F("Dati ricevuti: "));
            message += String((char *)data);

            addLog(LOG_LEVEL_INFO, message);
            Serial.printf("Question: %s\n", (char *)data);
        });

        // The master has read out outgoing data buffer
        // that buffer can be set with SPISlave.setData
        SPISlave.onDataSent([]() {
            String message = String(F("Status Sent: "));
            addLog(LOG_LEVEL_INFO, message);
            Serial.println("Risposta inviata al Master");
        });

        // status has been received from the master.
        // The status register is a special register that bot the slave and the master can write to and read from.
        // Can be used to exchange small data or status information
        SPISlave.onStatus([](uint32_t data) {
            String message = String(F("Status: "));
            message += String((char *)data);
            
            addLog(LOG_LEVEL_INFO, message);
            Serial.printf("Status: %u\n", data);
            SPISlave.setStatus(millis()); //set next status
        });

        // The master has read the status register
        SPISlave.onStatusSent([]() {
            String message = String(F("Status Sent: "));
            addLog(LOG_LEVEL_INFO, message);
            Serial.println("Status Sent");
        });

        // Setup SPI Slave registers and pins
        SPISlave.begin();

        // Set the status register (if the master reads it, it will read this value)
        SPISlave.setStatus(millis());

        // Sets the data registers. Limited to 32 bytes at a time.
        // SPISlave.setData(uint8_t * data, size_t len); is also available with the same limitation
        SPISlave.setData("Ask me a question!");

        success = true;
        break;
    }

    case PLUGIN_READ:
    {

        UserVar[event->BaseVarIndex] = 0;
        UserVar[event->BaseVarIndex + 1] = 0;

        success = true;
        break;
    }
    }
    return success;
}

//funzioni

//Start SPI SLave
void hspi_slave_begin(uint8_t status_len, void *arg);

//set the status register so the master can read it
void hspi_slave_setStatus(uint32_t status);

//set the data registers (max 32 bytes at a time)
void hspi_slave_setData(uint8_t *data, uint8_t len);

//set the callbacks
void hspi_slave_onData(void (*rxd_cb)(void *, uint8_t *, uint8_t));
void hspi_slave_onDataSent(void (*txd_cb)(void *));
void hspi_slave_onStatus(void (*rxs_cb)(void *, uint32_t));
void hspi_slave_onStatusSent(void (*txs_cb)(void *));

static void (*_hspi_slave_rx_data_cb)(void *arg, uint8_t *data, uint8_t len) = NULL;
static void (*_hspi_slave_tx_data_cb)(void *arg) = NULL;
static void (*_hspi_slave_rx_status_cb)(void *arg, uint32_t data) = NULL;
static void (*_hspi_slave_tx_status_cb)(void *arg) = NULL;
static uint8_t _hspi_slave_buffer[33];

void _hspi_slave_isr_handler(void *arg)
{
    uint32_t status;
    uint32_t istatus;

    istatus = SPIIR;

    if (istatus & (1 << SPII1))
    { //SPI1 ISR
        status = SPI1S;
        SPI1S &= ~(0x3E0); //disable interrupts
        SPI1S |= SPISSRES; //reset
        SPI1S &= ~(0x1F);  //clear interrupts
        SPI1S |= (0x3E0);  //enable interrupts

        if ((status & SPISRBIS) != 0 && (_hspi_slave_tx_data_cb))
        {
            _hspi_slave_tx_data_cb(arg);
        }
        if ((status & SPISRSIS) != 0 && (_hspi_slave_tx_status_cb))
        {
            _hspi_slave_tx_status_cb(arg);
        }
        if ((status & SPISWSIS) != 0 && (_hspi_slave_rx_status_cb))
        {
            uint32_t s = SPI1WS;
            _hspi_slave_rx_status_cb(arg, s);
        }
        if ((status & SPISWBIS) != 0 && (_hspi_slave_rx_data_cb))
        {
            uint8_t i;
            uint32_t data;
            _hspi_slave_buffer[32] = 0;
            for (i = 0; i < 8; i++)
            {
                data = SPI1W(i);
                _hspi_slave_buffer[i << 2] = data & 0xff;
                _hspi_slave_buffer[(i << 2) + 1] = (data >> 8) & 0xff;
                _hspi_slave_buffer[(i << 2) + 2] = (data >> 16) & 0xff;
                _hspi_slave_buffer[(i << 2) + 3] = (data >> 24) & 0xff;
            }
            _hspi_slave_rx_data_cb(arg, &_hspi_slave_buffer[0], 32);
        }
    }
    else if (istatus & (1 << SPII0))
    {                      //SPI0 ISR
        SPI0S &= ~(0x3ff); //clear SPI ISR
    }
    else if (istatus & (1 << SPII2))
    {
    } //I2S ISR
}

void hspi_slave_begin(uint8_t status_len, void *arg)
{
    status_len &= 7;
    if (status_len > 4)
    {
        status_len = 4; //max 32 bits
    }
    if (status_len == 0)
    {
        status_len = 1; //min 8 bits
    }

    pinMode(SS, SPECIAL);
    pinMode(SCK, SPECIAL);
    pinMode(MISO, SPECIAL);
    pinMode(MOSI, SPECIAL);

    SPI1S = SPISE | SPISBE | 0x3E0;
    SPI1U = SPIUMISOH | SPIUCOMMAND | SPIUSSE;
    SPI1CLK = 0;
    SPI1U2 = (7 << SPILCOMMAND);
    SPI1S1 = (((status_len * 8) - 1) << SPIS1LSTA) | (0xff << SPIS1LBUF) | (7 << SPIS1LWBA) | (7 << SPIS1LRBA) | SPIS1RSTA;
    SPI1P = (1 << 19);
    SPI1CMD = SPIBUSY;

    ETS_SPI_INTR_ATTACH(_hspi_slave_isr_handler, arg);
    ETS_SPI_INTR_ENABLE();
}

void hspi_slave_setStatus(uint32_t status)
{
    SPI1WS = status;
}

void hspi_slave_setData(uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint32_t out = 0;
    uint8_t bi = 0;
    uint8_t wi = 8;

    for (i = 0; i < 32; i++)
    {
        out |= (i < len) ? (data[i] << (bi * 8)) : 0;
        bi++;
        bi &= 3;
        if (!bi)
        {
            SPI1W(wi) = out;
            out = 0;
            wi++;
        }
    }
}

void hspi_slave_onData(void (*rxd_cb)(void *, uint8_t *, uint8_t))
{
    _hspi_slave_rx_data_cb = rxd_cb;
}

void hspi_slave_onDataSent(void (*txd_cb)(void *))
{
    _hspi_slave_tx_data_cb = txd_cb;
}

void hspi_slave_onStatus(void (*rxs_cb)(void *, uint32_t))
{
    _hspi_slave_rx_status_cb = rxs_cb;
}

void hspi_slave_onStatusSent(void (*txs_cb)(void *))
{
    _hspi_slave_tx_status_cb = txs_cb;
}

void SPISlaveClass::_data_rx(uint8_t *data, uint8_t len)
{
    if (_data_cb)
    {
        _data_cb(data, len);
    }
}
void SPISlaveClass::_status_rx(uint32_t data)
{
    if (_status_cb)
    {
        _status_cb(data);
    }
}
void SPISlaveClass::_data_tx(void)
{
    if (_data_sent_cb)
    {
        _data_sent_cb();
    }
}
void SPISlaveClass::_status_tx(void)
{
    if (_status_sent_cb)
    {
        _status_sent_cb();
    }
}
void SPISlaveClass::_s_data_rx(void *arg, uint8_t *data, uint8_t len)
{
    reinterpret_cast<SPISlaveClass *>(arg)->_data_rx(data, len);
}
void SPISlaveClass::_s_status_rx(void *arg, uint32_t data)
{
    reinterpret_cast<SPISlaveClass *>(arg)->_status_rx(data);
}
void SPISlaveClass::_s_data_tx(void *arg)
{
    reinterpret_cast<SPISlaveClass *>(arg)->_data_tx();
}
void SPISlaveClass::_s_status_tx(void *arg)
{
    reinterpret_cast<SPISlaveClass *>(arg)->_status_tx();
}

void SPISlaveClass::begin()
{
    hspi_slave_onData(&_s_data_rx);
    hspi_slave_onDataSent(&_s_data_tx);
    hspi_slave_onStatus(&_s_status_rx);
    hspi_slave_onStatusSent(&_s_status_tx);
    hspi_slave_begin(4, this);
}
void SPISlaveClass::setData(uint8_t *data, size_t len)
{
    if (len > 32)
    {
        len = 32;
    }
    hspi_slave_setData(data, len);
}
void SPISlaveClass::setStatus(uint32_t status)
{
    hspi_slave_setStatus(status);
}
void SPISlaveClass::onData(SpiSlaveDataHandler cb)
{
    _data_cb = cb;
}
void SPISlaveClass::onDataSent(SpiSlaveSentHandler cb)
{
    _data_sent_cb = cb;
}
void SPISlaveClass::onStatus(SpiSlaveStatusHandler cb)
{
    _status_cb = cb;
}
void SPISlaveClass::onStatusSent(SpiSlaveSentHandler cb)
{
    _status_sent_cb = cb;
}
