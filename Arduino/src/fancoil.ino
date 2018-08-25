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
#define RESET 6

#define POWER_OFF 0
#define MODE_AUTO 1
#define MODE_MIN 2
#define MODE_MAX 3
#define MODE_NIGHT 4

#define MIN_MODE 0
#define MAX_MODE 4

#define MAX_TEMP 280
#define MIN_TEMP 160

#define PARAM_ERROR 511
#define TEMP_ERROR 511
#define MODE_ERROR 255

void (*resetFunc)(void) = 0; //indirizzo di memoria bootloader

uint8_t spiData[7];
uint16_t fancoilData[4];
uint16_t fancoilTemp;
uint8_t fancoilMode;

uint8_t setTempFlag;
uint8_t setModeFlag;
uint8_t resetFlag;

uint8_t receiveCmd;
uint16_t receivedParam;


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
  resetFlag = 0;
  setTempFlag = 0;
  setModeFlag = 0;

  digitalWrite(6,HIGH); //imposta la resistenza di pullup sul pin collegato al fancoil
  pinMode(6, OUTPUT); //imposta come uscita il pin collegato al fancoil
  digitalWrite(6,HIGH); //imposta a valore alto il pin collegato al fancoil
  pinMode(7, OUTPUT); //imposta come uscita il pin che abilita la lettura da SPI

  Timer1.initialize(10000000L); // imposta timer watchdog a 10 secondi
  Timer1.stop(); // blocca il timer watchdog

  pinMode(MISO, OUTPUT);
  // bring in SPI slave mode
  SPCR |= _BV(SPE);
  SPCR |= 0b00001100;
  // enable interrupts
  //SPCR |= _BV(SPIE);

  Serial.begin(115200);
  Serial.println("Setup");
  Wire.begin(WIRE_ADDRESS);     // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
  Wire.onReceive(receiveEvent); // receive event
}

void loop()
{ 
  Serial.println("");
  Serial.print("Fancoil temp:");
  Serial.println(fancoilTemp);
  Serial.print("Fancoil mode:");
  Serial.println(fancoilMode);
  Serial.println("");
  
  
  //Timer1.resume();
  //Timer1.attachInterrupt(longWDT); //code to execute

  readFancoilData();

  
  if(setTempFlag) {
    uint16_t counter = 3;
    while((counter > 0) && (fancoilTemp != receivedParam)) {
      setTemp(receivedParam);
      readFancoilData();
      counter--;
    }
    setTempFlag = 0;
  }

  if(setModeFlag) {
    uint16_t counter = 3;
    while((counter > 0) && (fancoilMode != receivedParam)) {
      setMode(receivedParam);
      readFancoilData();
      counter--;
    }
    setModeFlag = 0;
  }

  if(resetFlag) {
    resetFunc();
    resetFlag = 0;
  }

  //Timer1.detachInterrupt();
  //Timer1.stop();

  delay(1000);
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  switch(receiveCmd) {

    case GET_TEMP: {
      I2CWriteTwoBytes(fancoilTemp);
      break;
    }

    case GET_MODE: {
      Wire.write(fancoilMode);
      break;
    }
      
    case SET_TEMP: {
      setTempFlag = 1;
      Wire.write(1);
      break;
    }

    case SET_MODE: {
      setModeFlag = 1;
      Wire.write(1);   
      break;
    }

    case RESET: {
      resetFlag = 1;
      Wire.write(1);
      break;
    }

  }
}

void receiveEvent(int numBytes) {
  if(Wire.available()) {
    receiveCmd = Wire.read();
  }
  if(Wire.available()) {
    receivedParam = I2CReadTwoBytes();
  }
}

void decodeData() {
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
      Serial.print("val:");
      Serial.println(val,BIN);
      flagTempMax = val & 0b00100000;
      
      if (val & 0b00001000)
        fancoilMode = MODE_AUTO;
      else if (val & 0b0100)
        fancoilMode = MODE_MIN;
      else if (val & 0b0010)
        fancoilMode = MODE_NIGHT;
      else if (val & 0b0001)
        fancoilMode = MODE_MAX;
      else if (!(val & 0b1111))
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
      fancoilTemp = MAX_TEMP + 5;  // valore inuscita di 30° per indicare temperatura MAX
    else
      fancoilTemp = MIN_TEMP -5;   // valore inuscita di 0° per indicare temperatura MIN
    break;
  }
  case 1:
  {
    fancoilTemp = 210;
    break;
  }
  case 2:
  {
    fancoilTemp = 200;
    break;
  }
  case 3:
  {
    fancoilTemp = 205;
    break;
  }
  case 4:
  {
    fancoilTemp = 190;
    break;
  }
  case 6:
  {
    fancoilTemp = 195;
    break;
  }
  case 8:
  {
    fancoilTemp = 180;
    break;
  }
  case 12:
  {
    fancoilTemp = 185;
    break;
  }
  case 16:
  {
    fancoilTemp = 170;
    break;
  }
  case 24:
  {
    fancoilTemp = 175;
    break;
  }
  case 32:
  {
    fancoilTemp = 160;
    break;
  }
  case 48:
  {
    fancoilTemp = 165;
    break;
  }
  case 64:
  {
    fancoilTemp = 220;
    break;
  }
  case 65:
  {
    fancoilTemp = 215;
    break;
  }
  case 256:
  {
    fancoilTemp = 280;
    break;
  }
  case 512:
  {
    fancoilTemp = 270;
    break;
  }
  case 768:
  {
    fancoilTemp = 275;
    break;
  }
  case 1024:
  {
    fancoilTemp = 260;
    break;
  }
  case 1536:
  {
    fancoilTemp = 265;
    break;
  }
  case 2048:
  {
    fancoilTemp = 250;
    break;
  }
  case 3072:
  {
    fancoilTemp = 255;
    break;
  }
  case 4096:
  {
    fancoilTemp = 240;
    break;
  }
  case 6144:
  {
    fancoilTemp = 245;
    break;
  }
  case 8192:
  {
    fancoilTemp = 230;
    break;
  }
  case 8256:
  {
    fancoilTemp = 225;
    break;
  }
  case 12288:
  {
    fancoilTemp = 235;
    break;
  }
  }
}

void setTemp(uint16_t t) {
  //mi assicuro che il valore di temp sia un multiplo di 5;
  uint8_t x = t / 5;
  uint16_t temp = x * 5;
   
  if((fancoilTemp != temp) && (temp <= MAX_TEMP) && (temp >= MIN_TEMP)) {
    uint16_t counter = 500;
    while ((fancoilTemp < temp) && (counter > 0)) {
      digitalWrite(7, LOW);
      while (!(SPSR & (1 << SPIF))) {}
      while (!(readPrefix() & 0b00001000)) {}
      digitalWrite(6, LOW);
      delayMicroseconds(5500);    
      digitalWrite(6, HIGH);
      readFancoilData();
      counter--;
    }
    while ((fancoilTemp > temp) && (counter > 0)) {
      digitalWrite(7, LOW);
      while (!(SPSR & (1 << SPIF))) {}
      while (!(readPrefix() & 0b00010000)) {}
      digitalWrite(6, LOW);
      delayMicroseconds(5500);
      digitalWrite(6, HIGH);
      readFancoilData();
      counter--;
    }
  }
}

void setMode(uint16_t mode) {

  if((fancoilMode != mode) && (mode >= MIN_MODE) && (mode <= MAX_MODE)) {

    if(mode == POWER_OFF) {
      uint16_t counter = 10; 
      while ((fancoilMode != mode) && (counter > 0)) {
        for(uint16_t i = 0; i < 50; i++) {
          digitalWrite(7, LOW);
          while (!(SPSR & (1 << SPIF))) {}
          while (!(readPrefix() & 0b01000000)) {}
          digitalWrite(6, LOW);
          delayMicroseconds(5500);  
          digitalWrite(6, HIGH);
        }
        readFancoilData();
        counter--;  
      }

    } else {  
      //accende il fancoil se spento
      if(fancoilMode == POWER_OFF) {   
        uint16_t counter = 50; 
        while ((fancoilMode == POWER_OFF) && (counter > 0)) {
          digitalWrite(7, LOW);
          while (!(SPSR & (1 << SPIF))) {}
          while (!(readPrefix() & 0b01000000)) {}
          digitalWrite(6, LOW);
          delay(1000);  //preme per 5 sec
          digitalWrite(6, HIGH);
          readFancoilData();
          counter--;  
        }
      }
      
      //imposta modalità
      uint16_t counter = 50; 

      while ((fancoilMode != mode) && (counter > 0)) {

        digitalWrite(7, LOW);
        while (!(SPSR & (1 << SPIF))) {}
        while (!(readPrefix() & 0b01000000)) {}
        digitalWrite(6, LOW);
        delayMicroseconds(5500);    
        digitalWrite(6, HIGH);
	      readFancoilData();
        uint8_t i = 5;
	      while((i>0) && (fancoilMode != mode)) {
           readFancoilData();
           i--;
	      }  

        counter--;
      }
    }
  } 
}

void readSPI() {
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

void reworkData(uint8_t *data, uint16_t *result) {
  
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

void printByte(uint8_t b) {
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

