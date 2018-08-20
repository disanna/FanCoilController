#include <Wire.h>
#include <Arduino.h>
#include <TimerOne.h>

#define WIRE_ADDRESS 1
#define GET_TEMP 0
#define GET_MODE 1
#define SET_TEMP 2
#define SET_MODE 3
#define INC_TEMP 4
#define DEC_TEMP 5

#define POWER_OFF 0
#define MODE_AUTO 1
#define MODE_MIN 2
#define MODE_MAX 3
#define MODE_NIGHT 4

#define MAX_TEMP 280
#define MIN_TEMP 160

void (*resetFunc)(void) = 0; //indirizzo di memoria bootloader

uint8_t spiData[7];
uint16_t fancoilData[4];
uint8_t receiveCmd;
uint16_t receivedParam;
uint8_t fancoilTemp;
uint8_t fancoilMode;

void longWDT(void)
{
    sei();

    //detach Timer1 interrupt e blocca timer
    Timer1.detachInterrupt();
    Timer1.stop();

    //flush, as Serial is buffered; and on hitting reset that buffer is cleared
    Serial.println("Errore di timeout");
    Serial.flush();
 
    //call to bootloader / code at address 0
    resetFunc();
}

void setup()
{
  Timer1.initialize(2000000); //2 second pulses
  Timer1.stop();

  receiveCmd = 255;
  receivedParam = 255;
  fancoilTemp = 255;
  fancoilMode = 255;

  pinMode(MISO, OUTPUT);
  // bring in SPI slave mode
  SPCR |= _BV(SPE);
  SPCR |= 0b00001100;
  // enable interrupts
  //SPCR |= _BV(SPIE);

  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  Serial.begin(115200);
  digitalWrite(6,HIGH);


  Wire.begin(WIRE_ADDRESS);     // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
  Wire.onReceive(receiveEvent); // receive event
}

void loop()
{ 
  Serial.print("Received cmd:");
  Serial.println(receiveCmd);
  Serial.print("Received param:");
  Serial.println(receivedParam);
  Serial.print("Fancoil temp:");
  Serial.println(fancoilTemp);
  Serial.print("Fancoil mode:");
  Serial.println(fancoilMode);
  
  Timer1.resume();
  //Timer1.start();
  Timer1.attachInterrupt(longWDT); //code to execute
  readFancoilData();
  Timer1.detachInterrupt();
  Timer1.stop();

  delay(100);
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  switch(receiveCmd) {
    case INC_TEMP: {
      setTemp(fancoilTemp + 5);
      break;
    }

    case DEC_TEMP: {
      setTemp(fancoilTemp-5);
      break;
    }

    case GET_TEMP: {
      I2CWriteTwoBytes(fancoilTemp);
      break;
    }

    case SET_TEMP: {
      uint16_t result = setTemp(receivedParam);
      break;
    }

    case GET_MODE: {
      Wire.write(fancoilMode);
      break;
    }

    case SET_MODE: {
      uint8_t result = setMode(receivedParam);
      break;
    }
  }
}

void receiveEvent(int numBytes)
{
  receiveCmd = Wire.read();
  if(Wire.available()) {
    receivedParam = I2CReadTwoBytes();
  } else {
    receivedParam = 255; //errore
  }
}

void decodeData()
{
  uint8_t flagTempMax, val1, val2 = 0;
  uint16_t valTot = 0;
  for (int i = 0; i < 4; i++)
  {
    uint8_t prefix = (fancoilData[i] >> 8) & 0b1111;
    uint8_t val = (fancoilData[i] >> 1) & 0b11111111;

    switch (prefix)
    {
    case 1:
    {
      flagTempMax = val & 0b00100000;
      if (val & 0b00001000)
        fancoilMode = MODE_AUTO;
      else if (val & 0b00000100)
        fancoilMode = MODE_MIN;
      else if (val & 0b00000010)
        fancoilMode = MODE_NIGHT;
      else if (val & 0b00000001)
        fancoilMode = MODE_MAX;
      else if (val == 0)
        fancoilMode = POWER_OFF;
      break;
    }
    case 2:
    {
      val1 = val;
      break;
    }
    case 4:
    {
      val2 = val;
      break;
    }
    case 8:
    {
      break;
    }
    }
  }
  valTot = val1 + val2 * 256;

  switch (valTot)
  {
  case 0:
  {
    if (flagTempMax)
      fancoilTemp = 300 - 100;  // valore inuscita di 30° per indicare temperatura MAX
    else
      fancoilTemp = 0;   // valore inuscita di 0° per indicare temperatura MIN
    break;
  }
  case 1:
  {
    fancoilTemp = 210 - 100;
    break;
  }
  case 2:
  {
    fancoilTemp = 200 - 100;
    break;
  }
  case 3:
  {
    fancoilTemp = 205 - 100;
    break;
  }
  case 4:
  {
    fancoilTemp = 190 - 100;
    break;
  }
  case 6:
  {
    fancoilTemp = 195 - 100;
    break;
  }
  case 8:
  {
    fancoilTemp = 180 - 100;
    break;
  }
  case 12:
  {
    fancoilTemp = 185 - 100;
    break;
  }
  case 16:
  {
    fancoilTemp = 170 - 100;
    break;
  }
  case 24:
  {
    fancoilTemp = 175 - 100;
    break;
  }
  case 32:
  {
    fancoilTemp = 160 - 100;
    break;
  }
  case 48:
  {
    fancoilTemp = 165 - 100;
    break;
  }
  case 64:
  {
    fancoilTemp = 220 - 100;
    break;
  }
  case 65:
  {
    fancoilTemp = 215 - 100;
    break;
  }
  case 256:
  {
    fancoilTemp = 280 - 100;
    break;
  }
  case 512:
  {
    fancoilTemp = 270 - 100;
    break;
  }
  case 768:
  {
    fancoilTemp = 275 - 100;
    break;
  }
  case 1024:
  {
    fancoilTemp = 260 - 100;
    break;
  }
  case 1536:
  {
    fancoilTemp = 265 - 100;
    break;
  }
  case 2048:
  {
    fancoilTemp = 250 - 100;
    break;
  }
  case 3072:
  {
    fancoilTemp = 255 - 100;
    break;
  }
  case 4096:
  {
    fancoilTemp = 240 - 100;
    break;
  }
  case 6144:
  {
    fancoilTemp = 245 - 100;
    break;
  }
  case 8192:
  {
    fancoilTemp = 230 - 100;
    break;
  }
  case 8256:
  {
    fancoilTemp = 225 - 100;
    break;
  }
  case 12288:
  {
    fancoilTemp = 235 - 100;
    break;
  }
  }
}

uint16_t setTemp(uint16_t temp)
{
  if((fancoilTemp != temp) && (temp < MAX_TEMP) && (temp > MIN_TEMP)) {
    uint8_t i = 0;
    while ((fancoilTemp < temp) && (i < 50)) {
      digitalWrite(7, LOW);
      while (!(SPSR & (1 << SPIF))) {}
      while (!(readPrefix() & 0b00001000)) {}
      digitalWrite(6, LOW);
      delayMicroseconds(5500);    
      digitalWrite(6, HIGH);
      readFancoilData();
      i++;
    }
    while ((fancoilTemp > temp) && (i < 50)) {
      digitalWrite(7, LOW);
      while (!(SPSR & (1 << SPIF))) {}
      while (!(readPrefix() & 0b00010000)) {}
      digitalWrite(6, LOW);
      delayMicroseconds(5500);
      digitalWrite(6, HIGH);
      readFancoilData();
      i++;
    }
  }
  if(fancoilTemp == temp) {
    return temp; 
  } else {
    return 255;  // errore
  }
}

uint8_t setMode(uint16_t mode) {
  // to do
}

void readSPI()
{
  digitalWrite(7, HIGH);
  digitalWrite(7, LOW);
  while (!(SPSR & (1 << SPIF)))
  {
  }
  digitalWrite(7, HIGH);
  SPDR = 0;
  delayMicroseconds(120);
  digitalWrite(7, LOW);
  while (!(SPSR & (1 << SPIF)))   //attende flag dat pronti
  {
  }
  spiData[0] = SPDR;              // legge primo byte
  while (!(SPSR & (1 << SPIF)))   //attende flag dat pronti
  {
  }
  spiData[1] = SPDR;              // legge secondo byte
  while (!(SPSR & (1 << SPIF)))   //attende flag dat pronti
  {
  }
  spiData[2] = SPDR;              // legge terzo byte
  while (!(SPSR & (1 << SPIF)))   //attende flag dat pronti
  {
  }
  spiData[3] = SPDR;              // legge quarto byte
  while (!(SPSR & (1 << SPIF))) //attende flag dat pronti
  {
  }
  spiData[4] = SPDR;              // legge quinto byte
  while (!(SPSR & (1 << SPIF))) //attende flag dat pronti
  {
  }
  spiData[5] = SPDR;              // legge sesto byte
  while (!(SPSR & (1 << SPIF))) //attende flag dat pronti
  {
  }
  spiData[6] = SPDR;              // legge settimo byte
  digitalWrite(7, HIGH);
}

void reworkData(uint8_t *data, uint16_t *result)
{
  
  //    0.0000.0000.0000
  //  data[0]  0000.0000
  //  data[1]  0000.0111
  //  data[2]  1111.1111
  //  data[3]  1122.2222
  //  data[4]  2222.2223
  //  data[5]  3333.3333
  //  data[6]  3333.4444
  /*
  Serial.println("data:");
  for(int i=0;i<7;i++) {
    Serial.print(data[i],DEC);
    Serial.print("-");
    for(int k=0;k<8;k++) {
      if((data[i] << k) & 0b10000000)
        Serial.print("1");
      else
        Serial.print("0");
    }
    Serial.print(".");
  }
  Serial.println("");
  */
  uint16_t r = 0;
  uint16_t d = 0;
  d = data[0] << 5;
  r = r | d;
  d = (data[1] & 0b11111000) >> 3;
  r = r | d;
  result[0] = r;

  r = 0;
  d = 0;
  d = (data[1] & 0b111) << 10;
  r = r | d;
  d = data[2] << 2;
  r = r | d;
  d = (data[3] & 0b11000000) >> 6;
  r = r | d;
  result[1] = r;

  r = 0;
  d = 0;
  d = (data[3] & 0b00111111) << 7;
  r = r | d;
  d = (data[4] & 0b11111110) >> 1;
  r = r | d;
  result[2] = r;

  r = 0;
  d = 0;
  d = (data[4] & 0b1) << 12;
  r = r | d;
  d = data[5] << 4;
  r = r | d;
  d = (data[6] & 0b11110000) >> 4;
  r = r | d;
  result[3] = r;

  /*
  Serial.println("result:");
  for(int i=0;i<4;i++) {
    for(int k=0;k<16;k++){
      if((result[i]<< k) & 0b1000000000000000)
        Serial.print("1");
      else
        Serial.print("0");
    }
    Serial.print(".");
  }
  Serial.println("");
  */
}


void readFancoilData() {
  readSPI();
  reworkData(spiData, fancoilData);
  decodeData();
}

byte readPrefix() {
    digitalWrite(7, HIGH);
    SPDR = 0;
    delayMicroseconds(120);
    digitalWrite(7, LOW);
    while (!(SPSR & (1 << SPIF))) {}
    return SPDR;
}

void printByte(uint8_t b)
{
  for (int i = 7; i >= 0; i--)
  {
    if ((b >> i) & 1)
      Serial.print("1");
    else
      Serial.print("0");
  }
  Serial.println("");
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

