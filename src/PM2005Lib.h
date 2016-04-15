/*
  PM2005Lib.h - Library control and read-out of
  the PM2005 laser particle sensor module.
  Created by Jan Wagner, April 15, 2016.
  Released into the public domain.
*/

#ifndef PM2005Lib_h
#define PM2005Lib_h

#include <Arduino.h>
#include "Stream.h"

#define PM2005LIB_VERSION "1.0.0"
#define PM2005_BAUD 9600

class PM2005Lib {

  public:
    PM2005Lib(Stream& device);
    PM2005Lib(Stream& device, Stream& user);

  public:
    void init();

    void measure();
    void printReadings();

    void printAlarmflags();
    void printCalibrationflags();
    void printWorkstatus();

  public:
    void communicate(const unsigned char* txbuf, const int txlen, unsigned char* rxbuf, const int rxlen);

  public:
    uint32_t counts_05um;        // 0.5 micrometer particle count in particles/litre
    uint32_t counts_2um;         // 2.5 micrometer particle count in particles/litre
    uint32_t counts_10um;        // 10 micrometer particle count in particles/litre
    uint16_t concentration_2um;  // PM2.5 in micrograms/cubic meter
    uint16_t concentration_10um; // PM10 in micrograms/cubic meter

    unsigned char alarm_flags, calibration_flags, workstatus;

  private:
    void printlnHex(const unsigned char* data, const int len); // like Serial.print(data,HEX) but for an array, and shows leading zeroes.
    void communicate(const unsigned char txdata[], const int rxlen); // like communicate() but without user-provided buffers

  private:
    unsigned char _rx_buf[32];
    Stream* _sdevice;  // the serial interface attached to the PM2005
    Stream* _suser;    // optional serial interface attached elsewhere, used for printouts
};

#endif
