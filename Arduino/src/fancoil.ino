#include <Wire.h>
#include <Arduino.h>

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


void setup()
{
  Serial.begin(9600);

  Wire.begin(WIRE_ADDRESS);     // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register even
}

void loop()
{ 
  
  Serial.println("ciao");
  uint8_t v = 0;
  v= Wire.read();
  Serial.println("wire.read dentro loop= ");
  Serial.print(v);
  delay(100);
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Serial.println("requestEvent");
}