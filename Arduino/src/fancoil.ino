#include <Wire.h>
#include <Arduino.h>

#define WIRE_ADDRESS 2
#define GET_TEMP 0
#define GET_MODE 1
#define SET_TEMP 2
#define SET_MODE 3

#define POWER_OFF 0
#define MODE_AUTO 1
#define MODE_MIN 2
#define MODE_MAX 3
#define MODE_NIGHT 4

uint8_t cmd;
uint16_t receivedParam;
uint8_t spiData[7];
uint16_t fancoilData[4];
uint8_t fancoilTemp, fancoilMode;

void setup()
{
  cmd = 0;
  pinMode(MISO, OUTPUT);
  // bring in SPI slave mode
  SPCR |= _BV(SPE);
  SPCR |= 0b00001100;
  // enable interrupts
  //SPCR |= _BV(SPIE);

  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  digitalWrite(6,HIGH);
  Serial.begin(115200);

  Wire.begin(WIRE_ADDRESS);     // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
  Wire.onReceive(receiveEvent); // receive event
}

void loop()
{
  byte incomingByte = 0;
  //readFancoilData();
  if (Serial.available() > 0) {
  // read the incoming byte:
    incomingByte = Serial.read();
    if(incomingByte=='s') {
      setTemp(fancoilTemp+5);
    }
    if(incomingByte=='g') {
      setTemp(fancoilTemp-5);
    }
  }

  
  Serial.print("Temp= ");
  Serial.println(fancoilTemp);
  Serial.print("Modalità= ");
  Serial.println(fancoilMode);
  delay(100);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Serial.print("requestEvent");
  switch (cmd)
  {
  case GET_TEMP:
  {
    decodeData();
    Wire.write(fancoilTemp);
    Serial.println("Temp= " + fancoilTemp);
    break;
  }
  case SET_TEMP:
  {
    decodeData();
    if((fancoilTemp != receivedParam) && (receivedParam >= 0) && (receivedParam <= 300)) {
      setTemp(receivedParam);
      Serial.println("setTemp= " + receivedParam);
    }
    break;
  }


  }
}

void receiveEvent(int numBytes)
{
  Serial.print("receiveEvent");
  if (Wire.available() > 0) {
    cmd = Wire.read();
    receivedParam = Wire.read()*256;
    receivedParam += Wire.read();
    Serial.print("receiveEvent:cmd=");
    Serial.print(cmd);
    Serial.print("param=");
    Serial.println(receivedParam);
  }
  else
    cmd = 0;
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

void setTemp(uint16_t temp)
{
  Serial.print("setTemp: ");
  Serial.println(temp);
  uint8_t i = 0;
  while ((fancoilTemp < temp) && (i < 50))
  {
    digitalWrite(7, LOW);
    while (!(SPSR & (1 << SPIF))) {}
    while (!(readPrefix() & 0b00001000)) {}
    digitalWrite(6, LOW);
    delayMicroseconds(5500);    
    digitalWrite(6, HIGH);
    readFancoilData();
    i++;
  }
  while ((fancoilTemp > temp) && (i < 50))
  {
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
