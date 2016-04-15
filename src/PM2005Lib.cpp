/*
  PM2005Lib.c - Library control and read-out of
  the PM2005 laser particle sensor module.
  Created by Jan Wagner, April 15, 2016.
  Released into the public domain.

  Operating notes

  1) The PM2005 supports four different measurement modes: single, continous,
     dynamic, and timing. This library currently implements only continous mode.

  2) The PM2005 is extremely picky about its 5.0V supply voltage.
     If the 5V supply falls below ~4.6V the PM2005 returns measurements of 1000 ug/m3.
     The datasheet says the 5V supply should be within 4.9V to 5.1V.
     This means the Arduino Nano "+5V" pin is unsuitable, it is at 4.7V.

  3) The PM2005 measures particle counts/liter for 0.5 um, 2.5 um, and 10 um particles.
     The PM2005 estimates concentrations PM2.5 and PM10 in micrograms/m^3 (0-1000 ug/m^3).
     The PM2005 does not estimate PM0.3; PM0.3 are fine particles already included in PM2.5.

  Implementation notes

  1) The PM2005 interface commands are poorly documented. Continous mode
     appears to be enabled (also?) in command 0x03 by writing time 0xFFFB
     rather than the datasheet value of 0xFFFF.

  2) An undocumented command 0x0B 0x1F is used by the company's demo program
     PM2005_TestProgram_V1.0.exe to read the device serial number. The serial
     response binary data and what PM2005_TestProgram reports as the final
     serial number look somehow very different though...
*/

#include "PM2005Lib.h"

#define PM2005_DEBUG  // enable some debug printouts

// Requests to the PM2005, see datasheet
static const unsigned char pm2005_req_close[] = {0x11, 0x03, 0x0C, 0x01, 0x1E, 0xC1};
static const unsigned char pm2005_req_serial[] = {0x11, 0x01, 0x1F, 0xCF};
static const unsigned char pm2005_req_open[] = {0x11, 0x03, 0x0C, 0x02, 0x1E, 0xC0};
static const unsigned char pm2005_req_continuousmode[] = {0x11, 0x03, 0x0D, 0xFF, 0xFB, 0xE5};
static const unsigned char pm2005_req_counts_ugm[] = {0x11, 0x02, 0x0B, 0x01, 0xE1};
static const unsigned char pm2005_req_counts_pcs[] = {0x11, 0x01, 0x0B, 0xE3};
static const int pm2005_resp_close_len = 5;
static const int pm2005_resp_serial_len = 14;
static const int pm2005_resp_open_len = 5;
static const int pm2005_resp_continuousmode_len = 6;
static const int pm2005_resp_counts_ugm_len = 20;
static const int pm2005_resp_counts_pcs_len = 20;

PM2005Lib::PM2005Lib(Stream& device) {
    _sdevice = &device;
    _suser = NULL;
}

PM2005Lib::PM2005Lib(Stream& device, Stream& user) {
    _sdevice = &device;
    _suser = &user;
}

void PM2005Lib::init()
{
    // Initialize the PM2005 for carrying out "continous mode" measurements
    communicate(pm2005_req_close,          pm2005_resp_close_len);
    communicate(pm2005_req_serial,         pm2005_resp_serial_len);
    communicate(pm2005_req_continuousmode, pm2005_resp_continuousmode_len);
    communicate(pm2005_req_open,           pm2005_resp_open_len);
}

void PM2005Lib::measure()
{
    // Send a particle count readout request to PM2005
    communicate(pm2005_req_counts_pcs, pm2005_resp_counts_pcs_len);

    // Parse the PM2005 reply
    counts_05um  = _rx_buf[6] + (((uint32_t)_rx_buf[5])<<8)
                    + (((uint32_t)_rx_buf[4])<<16)
                    + (((uint32_t)_rx_buf[3])<<24);
    counts_2um  = _rx_buf[10] + (((uint32_t)_rx_buf[9])<<8)
                    + (((uint32_t)_rx_buf[8])<<16)
                    + (((uint32_t)_rx_buf[7])<<24);
    counts_10um = _rx_buf[14] + (((uint32_t)_rx_buf[13])<<8)
                    + (((uint32_t)_rx_buf[12])<<16)
                    + (((uint32_t)_rx_buf[11])<<24);

    // Send a particle concentration readout request to PM2006
    communicate(pm2005_req_counts_ugm, pm2005_resp_counts_ugm_len);

    // Parse the PM2005 reply -- it includes also alarm/status/work flags
    concentration_2um  = _rx_buf[6] + 256U*((uint16_t)_rx_buf[5]);
    concentration_10um = _rx_buf[10] + 256U*((uint16_t)_rx_buf[9]);
    alarm_flags        = _rx_buf[15];
    //undocumented_flags = _rx_buf[16];
    calibration_flags  = _rx_buf[17];
    workstatus         = _rx_buf[18];
}

void PM2005Lib::printReadings()
{
    _suser->print("0.5um [pcs/L]: ");
    _suser->println(counts_05um,DEC);
    _suser->print("2.5um [pcs/L]: ");
    _suser->println(counts_2um,DEC);
    _suser->print("10um [pcs/L]: ");
    _suser->println(counts_10um,DEC);

    _suser->print("PM2.5 [ug/m^3] : ");
    _suser->println(concentration_2um,DEC);
    _suser->print("PM10 [ug/m^3]  : ");
    _suser->println(concentration_10um,DEC);
}

void PM2005Lib::printAlarmflags()
{
    if (alarm_flags & 1) {
        _suser->println("pm2005: high RPM");
    }
    if (alarm_flags & 2) {
        _suser->println("PM2005: low RPM");
    }
    if (alarm_flags & 4) {
        _suser->println("PM2005: high Temp");
    }
    if (alarm_flags & 8) {
        _suser->println("PM2005: low Temp");
    }
    if (alarm_flags & 16) {
        _suser->println("PM2005: high sensit.");
    }
    if (alarm_flags & 32) {
        _suser->println("PM2005: low sensit.");
    }
    if (alarm_flags & 64) {
        _suser->println("PM2005: high Ild");
    }
    if (alarm_flags & 128) {
        _suser->println("PM2005: low Ild");
    }
}

void PM2005Lib::printCalibrationflags()
{
    if (calibration_flags & 1) {
        _suser->println("PM2005: uncal., normal T");
    }
    if (calibration_flags & 2) {
        _suser->println("PM2005: uncal., low T");
    }
    if (calibration_flags & 4) {
        _suser->println("PM2005: uncal., high T");
    }
}

void PM2005Lib::printWorkstatus()
{
    if (workstatus == 3) {
        _suser->println("PM2005: stopped");
    }
    if (workstatus == 1) {
        _suser->println("PM2005: measuring");
    }
    if (workstatus == 128) {
        _suser->println("PM2005: done");
    }
}


void PM2005Lib::communicate(const unsigned char txdata[], const int rxlen)
{
    communicate(txdata, sizeof(txdata), _rx_buf, rxlen);
}

void PM2005Lib::communicate(const unsigned char txbuf[], const int txlen, unsigned char rxbuf[], const int rxlen)
{
    int nrxd = 0;

    _sdevice->write(txbuf, txlen);
    while (nrxd < rxlen) {
        if (_sdevice->available()) {
            rxbuf[nrxd] = _sdevice->read();
            nrxd++;
        }
    }

    #ifdef PM2005_DEBUG
    if (_suser) {
        _suser->print("pm2005: ");
        printlnHex(rxbuf, rxlen);
    }
    #endif
}

void PM2005Lib::printlnHex(const unsigned char* data, const int len)
{
    if (!_suser) {
        return;
    }

    for (int i=0; i<len; i++) {
        char nibble, hexchar;

        nibble = (data[i] >> 4) & 0x0f;
        if (nibble <= 9) {
            hexchar = '0' + nibble;
        } else {
            hexchar = 'A' + (nibble-10);
        }
        _suser->print(hexchar);

        nibble = data[i] & 0x0f;
        if (nibble <= 9) {
            hexchar = '0' + nibble;
        } else {
            hexchar = 'A' + (nibble-10);
        }
        _suser->print(hexchar);

        _suser->print(' ');
    }
    _suser->println("");
}
