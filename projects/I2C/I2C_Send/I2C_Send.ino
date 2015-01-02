/*
* I2C_Master
* Sends I2C data bytes to device # address
*/

#include <Wire.h>

const int address = 4; // the address to be used by the communicating devices

void setup()
{
  Wire.begin(); // join i2c bus (address optional for master)
}

byte x = 0;

void loop()
{
  Wire.beginTransmission(address); // transmit to device #4
  Wire.write("x is ");        // sends five bytes
  Wire.write(x);              // sends one byte
  Wire.endTransmission();    // stop transmitting

  x++;
  delay(500);
}
