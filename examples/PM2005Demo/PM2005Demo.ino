//--Arduino Nano-----------------------------------------
// pin 8 to PM2005_TX
// pin 9 to PM2005_RX
// pin '5V'  to PM2005 +5V_In
// pin 'GND' to PM2005 GND
// pin 'Vin' to an external supply (>7V)
//
// Note:
// The PM2005 is picky and its supply voltage should
// be between 4.9V and 5.1V. If the Arduino is connected
// just by USB, then the '5V' pin outputs ~4.7V which is
// too low for the PM2005. One solution is to use an
// external supply to the Arduino 'Vin' pin.
//-------------------------------------------------------
#include <SoftwareSerial.h>
#include <PM2005Lib.h>

SoftwareSerial pm2005Serial(8, 9);
PM2005Lib pm2005(/*PM2005 device*/pm2005Serial, /*user message device*/Serial);

void setup() {
  // put your setup code here, to run once:

  // Initialize the USB serial port
  Serial.begin(115200);
  Serial.println("PM2005 TEST PROGRAM");
  Serial.println(PM2005LIB_VERSION);

  // Open the (software)serial port to the PM2005
  pm2005Serial.begin(PM2005_BAUD);

  // Initialize the PM2005 and start continous measurements
  pm2005.init();
}

void loop() {
  // put your main code here, to run repeatedly:

  // Read out PM2005 measurement data and print current readings
  pm2005.measure();
  pm2005.printReadings();

  // Can also access individual readings
  if (pm2005.concentration_2um > 500) {
    Serial.println("Unhealthy PM2.5 concentrations!");
  }
  if (pm2005.concentration_10um > 500) {
    Serial.println("Unhealthy PM10 concentrations!");
  }

  // Wait some time (e.g. 3 seconds) until next readout
  delay(3000);
}
