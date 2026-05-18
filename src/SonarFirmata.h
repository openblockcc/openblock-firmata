/*
  SonarFirmata.h - Firmata library
  Copyright (C) 2023 OpenBlock.cc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  Last updated by ArthurZheng: 2023.07.21
*/

#ifndef SonarFirmata_h
#define SonarFirmata_h

#include <ConfigurableFirmata.h>
#include <FirmataFeature.h>
#include <Ultrasonic.h>

#define SONAR_DATA 0x50

#define SONAR_UINT_CM 0
#define SONAR_UINT_INC 1

class SonarFirmata : public FirmataFeature
{
public:
    SonarFirmata();
    boolean handlePinMode(byte pin, int mode);
    void handleCapability(byte pin);
    boolean handleSysex(byte command, byte argc, byte *argv);
    void reset();
    void report();

private:
    boolean performSonarTransfer(byte trigPin, byte echoPin, byte unit);
    void attach(byte trigPin, byte echoPin);
    void detach();
    int _trigPin;
    int _echoPin;
    Ultrasonic *_sonar;
};

/*
 * ServoFirmata.cpp
 * Copied here as a hack to avoid having to include Servo.h in all sketch files that
 * include ConfigurableFirmata.h
 */

SonarFirmata::SonarFirmata()
{
    _sonar = NULL;
    _trigPin = -1;
    _echoPin = -1;
}

boolean SonarFirmata::handlePinMode(byte pin, int mode)
{
    return true;
}

void SonarFirmata::handleCapability(byte pin)
{
}

boolean SonarFirmata::handleSysex(byte command, byte argc, byte *argv)
{
    switch (command)
    {
    case SONAR_DATA:
        if (argc < 3)
        {
            Firmata.sendString(F("Error in Sonar command: Not enough parameters"));
            return false;
        }
        return performSonarTransfer(argv[0], argv[1], argv[2]);
    }
    return false;
}

boolean SonarFirmata::performSonarTransfer(byte trigPin, byte echoPin, byte unit)
{
    int dist = 0;

    attach(trigPin, echoPin);

    if (unit == SONAR_UINT_CM)
    {
        dist = (uint32_t)_sonar->read(CM) * 100;
    }
    else if (unit == SONAR_UINT_INC)
    {
        dist = (uint32_t)_sonar->read(INC) * 100;
    }
    else
    {
        return false;
    }

    Firmata.startSysex();
    Firmata.write(SONAR_DATA);
    Firmata.write(trigPin);
    Firmata.write(echoPin);
    Firmata.write(unit);
    Firmata.sendPackedUInt32(dist);
    Firmata.endSysex();

    return true;
}

void SonarFirmata::attach(byte trigPin, byte echoPin)
{
    if (!_sonar || _trigPin != trigPin || _echoPin != echoPin)
    {
        delete _sonar;
        _sonar = new Ultrasonic(trigPin, echoPin);
        _trigPin = trigPin;
        _echoPin = echoPin;
    }
}

void SonarFirmata::detach()
{
    if (_sonar)
    {
        delete _sonar;
        _sonar = NULL;
    }
}

void SonarFirmata::reset()
{
    detach();
}

#endif /* ServoFirmata_h */
