/*
 * ConfigurableFirmata standard example file, for serial communication.
 */

#include <ConfigurableFirmata.h>

// Use this to enable WIFI instead of serial communication. Tested on ESP32, but should also
// work with Wifi-enabled Arduinos
// #define ENABLE_WIFI

#ifdef ENABLE_WIFI
const char* ssid     = "your-ssid";
const char* password = "your-password";
const int NETWORK_PORT = 27016;
#endif

// Use these defines to easily enable or disable certain modules

// #define ENABLE_ONE_WIRE

// Note that the SERVO module currently is not supported on ESP32. So either disable this or patch the library
#ifndef ESP32
#define ENABLE_SERVO 
#endif

#if defined(__AVR__)
#define FIRMATA_BAUDRATE 57600
#else
#define FIRMATA_BAUDRATE 115200
#endif

// #define ENABLE_ACCELSTEPPER

// This is rarely used
// #define ENABLE_BASIC_SCHEDULER
#define ENABLE_SERIAL
#define ENABLE_I2C
#define ENABLE_SPI
#define ENABLE_ANALOG
#define ENABLE_DIGITAL
#define ENABLE_DHT
#define ENABLE_FREQUENCY

#define ENABLE_SONAR
#define ENABLE_BUZZER


#ifdef ENABLE_DIGITAL
#include <DigitalInputFirmata.h>
DigitalInputFirmata digitalInput;

#include <DigitalOutputFirmata.h>
DigitalOutputFirmata digitalOutput;
#endif

#ifdef ENABLE_ANALOG
#include <AnalogInputFirmata.h>
AnalogInputFirmata analogInput;

#include <AnalogOutputFirmata.h>
AnalogOutputFirmata analogOutput;
#endif


#ifdef ENABLE_WIFI
#include <WiFi.h>
#include "utility/WiFiClientStream.h"
#include "utility/WiFiServerStream.h"
WiFiServerStream serverStream(NETWORK_PORT);
#endif

#ifdef ENABLE_I2C
#include <Wire.h>
#include <I2CFirmata.h>
I2CFirmata i2c;
#endif

#ifdef ENABLE_SPI
#include <Wire.h>
#include <SpiFirmata.h>
SpiFirmata spi;
#endif

#ifdef ENABLE_ONE_WIRE
#include <OneWireFirmata.h>
OneWireFirmata oneWire;
#endif

#ifdef ENABLE_SERIAL
#include <SerialFirmata.h>
SerialFirmata serial;
#endif

#ifdef ENABLE_DHT
#include <DhtFirmata.h>
DhtFirmata dhtFirmata;
#endif

#ifdef ENABLE_SONAR
#include "src/SonarFirmata.h"
SonarFirmata sonarFirmata;
#endif

#ifdef ENABLE_BUZZER
#include "src/BuzzerFirmata.h"
BuzzerFirmata buzzerFirmata;
#endif

#include <FirmataExt.h>
FirmataExt firmataExt;

#ifdef ENABLE_SERVO
#include <Servo.h>
#include <ServoFirmata.h>
ServoFirmata servo;
#endif

#include <FirmataReporting.h>
FirmataReporting reporting;

#ifdef ENABLE_ACCELSTEPPER
#include <AccelStepperFirmata.h>
AccelStepperFirmata accelStepper;
#endif

#ifdef ENABLE_FREQUENCY
#include <Frequency.h>
Frequency frequency;
#endif

#ifdef ENABLE_BASIC_SCHEDULER
// The scheduler allows to store scripts on the board, however this requires a kind of compiler on the client side.
// When running dotnet/iot on the client side, prefer using the FirmataIlExecutor module instead
#include <FirmataScheduler.h>
FirmataScheduler scheduler;
#endif

void systemResetCallback()
{
  for (byte i = 0; i < TOTAL_PINS; i++) {
    if (IS_PIN_ANALOG(i)) {
      Firmata.setPinMode(i, PIN_MODE_ANALOG);
    } else if (IS_PIN_DIGITAL(i)) {
      Firmata.setPinMode(i, PIN_MODE_OUTPUT);
    }
  }
  firmataExt.reset();
}

void initTransport()
{
  // Uncomment to save a couple of seconds by disabling the startup blink sequence.
  // Firmata.disableBlinkVersion();
#ifdef ENABLE_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  pinMode(VERSION_BLINK_PIN, OUTPUT);
  bool pinIsOn = false;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    pinIsOn = !pinIsOn;
    digitalWrite(VERSION_BLINK_PIN, pinIsOn);
  }
  Firmata.begin(serverStream);
  Firmata.blinkVersion(); // Because the above doesn't do it.
#else 
  Firmata.begin(FIRMATA_BAUDRATE);
#endif
    
}

void initFirmata()
{
  // Set firmware name and version. The name is automatically derived from the name of this file.
  // Firmata.setFirmwareVersion(FIRMATA_FIRMWARE_MAJOR_VERSION, FIRMATA_FIRMWARE_MINOR_VERSION);
  // The usage of the above shortcut is not recommended, since it stores the full path of the file name in a 
  // string constant, using both flash and ram. 
  Firmata.setFirmwareNameAndVersion("ConfigurableFirmata", FIRMATA_FIRMWARE_MAJOR_VERSION, FIRMATA_FIRMWARE_MINOR_VERSION);

#ifdef ENABLE_DIGITAL
  firmataExt.addFeature(digitalInput);
  firmataExt.addFeature(digitalOutput);
#endif
	
#ifdef ENABLE_ANALOG
  firmataExt.addFeature(analogInput);
  firmataExt.addFeature(analogOutput);
#endif
	
#ifdef ENABLE_SERVO
  firmataExt.addFeature(servo);
#endif
	
#ifdef ENABLE_I2C
  firmataExt.addFeature(i2c);
#endif
	
#ifdef ENABLE_ONE_WIRE
  firmataExt.addFeature(oneWire);
#endif
	
#ifdef ENABLE_SERIAL
  firmataExt.addFeature(serial);
#endif
	
#ifdef ENABLE_BASIC_SCHEDULER
  firmataExt.addFeature(scheduler);
#endif
	
  firmataExt.addFeature(reporting);
#ifdef ENABLE_SPI
  firmataExt.addFeature(spi);
#endif
#ifdef ENABLE_ACCELSTEPPER
  firmataExt.addFeature(accelStepper);
#endif
	
#ifdef ENABLE_DHT
  firmataExt.addFeature(dhtFirmata);
#endif

#ifdef ENABLE_SONAR
  firmataExt.addFeature(sonarFirmata);
#endif

#ifdef ENABLE_BUZZER
  firmataExt.addFeature(buzzerFirmata);
#endif

#ifdef ENABLE_FREQUENCY
  firmataExt.addFeature(frequency);
#endif

  Firmata.attach(SYSTEM_RESET, systemResetCallback);
}

void setup()
{
	initFirmata();
	initTransport();
	Firmata.sendString(F("Booting device. Stand by..."));

	Firmata.parse(SYSTEM_RESET);
}

void loop()
{
  while(Firmata.available()) {
    Firmata.processInput();
    if (!Firmata.isParsingMessage()) {
      break;
    }
  }

  firmataExt.report(reporting.elapsed());
#ifdef ENABLE_WIFI
  serverStream.maintain();
#endif
}
