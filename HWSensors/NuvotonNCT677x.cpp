/*
 *  NCT677x.c
 *  HWSensors
 *
 *  Based on code from Open Hardware Monitor project by Michael Möller (C) 2011
 *  Copyright (C) 2011-2012 THe KiNG and mozodojo. All Rights Reserved (R)
 *  
 *
 */

/*
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 
 http://www.mozilla.org/MPL/
 
 Software distributed under the License is distributed on an "AS IS" basis,
 WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 for the specific language governing rights and limitations under the License.
 
 The Original Code is the Open Hardware Monitor code.
 
 The Initial Developer of the Original Code is 
 Michael Möller <m.moeller@gmx.ch>.
 Portions created by the Initial Developer are Copyright (C) 2010-2011
 the Initial Developer. All Rights Reserved.
 
 Contributor(s):
 
 Alternatively, the contents of this file may be used under the terms of
 either the GNU General Public License Version 2 or later (the "GPL"), or
 the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 in which case the provisions of the GPL or the LGPL are applicable instead
 of those above. If you wish to allow use of your version of this file only
 under the terms of either the GPL or the LGPL, and not to allow others to
 use your version of this file under the terms of the MPL, indicate your
 decision by deleting the provisions above and replace them with the notice
 and other provisions required by the GPL or the LGPL. If you do not delete
 the provisions above, a recipient may use your version of this file under
 the terms of any one of the MPL, the GPL or the LGPL.
 
 */

#include <architecture/i386/pio.h>
#include "FakeSMC.h"
#include "NuvotonNCT677x.h"

#define Debug FALSE

#define LogPrefix "NCT677x: "
#define DebugLog(string, args...)   do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)  do { IOLog (LogPrefix string "\n", ## args); } while(0)

#define super SuperIOMonitor
OSDefineMetaClassAndStructors(NCT677x, SuperIOMonitor)

UInt8 NCT677x::readByte(UInt16 reg)
{
  UInt8 bank = reg >> 8;
  UInt8 regi = reg & 0xFF;

  outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
  outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
  outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);

  return inb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET));
}

void NCT677x::writeByte(UInt16 reg, UInt8 value)
{
  UInt8 bank = reg >> 8;
  UInt8 regi = reg & 0xFF;

  outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
  outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
  outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);
  outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), value);
}

long NCT677x::readTemperature(unsigned long index)
{
  if (index < 2) {

    int value = readByte(NUVOTON_TEMPERATURE_REG[index]) << 1;

    float t = 0.5f * (float)value;

    return temperature[index] = t <= 125 && t >= -55 ? t : 0;
  }

  return 0;
}

long NCT677x::readVoltage(unsigned long index)
{
  if (index < 9) {

    float value = readByte(NUVOTON_VOLTAGE_REG[index]) * (NUVOTON_VOLTAGE_SCALE[index]);

    bool valid = value > 0;

    // check if battery voltage monitor is enabled
    if (valid && NUVOTON_VOLTAGE_REG[index] == NUVOTON_VOLTAGE_VBAT_REG) {
      valid = (readByte(0x5D) & 0x01) > 0;
    }

    return valid ? value : 0;
  }

  return 0;
}

long NCT677x::readTachometer(unsigned long index)
{
  if (index < 5) {

    UInt8 high = readByte(NUVOTON_FAN_RPM_REG[index]);
    UInt8 low  = readByte(NUVOTON_FAN_RPM_REG[index] + 1);

    int value = (high << 8) | low;

    return value > minFanRPM ? value : 0;
  }

  return 0;
}

void NCT677x::enter()
{
  outb(registerPort, 0x87);
  outb(registerPort, 0x87);
}

void NCT677x::exit()
{
  outb(registerPort, 0xAA);
}

bool NCT677x::probePort()
{
  UInt8 id        = listenPortByte(SUPERIO_CHIP_ID_REGISTER);
  UInt8 revision  = listenPortByte(SUPERIO_CHIP_REVISION_REGISTER);

  if (id == 0 || id == 0xff || revision == 0 || revision == 0xff) {
    return false;
  }

  switch (id) {
    case 0xB4:
      switch (revision & 0xF0) {
        case 0x70:
          model = NCT6771F;
          minFanRPM = (int)(1.35e6 / 0xFFFF);
          break;
      } break;
    case 0xC3:
      switch (revision & 0xF0) {
        case 0x30:
          model = NCT6776F;
          minFanRPM = (int)(1.35e6 / 0x1FFF);
          break;
      } break;
  }

  if (!model) {
    WarningLog("Found unsupported chip ID=0x%x REVISION=0x%x", id, revision);
    return false;
  }

  selectLogicalDevice(NUVOTON_HARDWARE_MONITOR_LDN);

  if (!getLogicalDeviceAddress()) {
    WarningLog("Can't get monitoring logical device address");
    return false;
  }

  return true;
}

bool NCT677x::startPlugin()
{
  InfoLog("Found Nuvoton %s", getModelName());

  OSDictionary  *list          = OSDynamicCast(OSDictionary, getProperty("Sensors Configuration"));
  OSDictionary  *configuration = list ? OSDynamicCast(OSDictionary, list->getObject(getModelName())) : 0;

  if (list && !configuration) 
    configuration = OSDynamicCast(OSDictionary, list->getObject("Default"));

  // Fans
  for (int i = 0; i < 5; i++) {

    OSString  *name = 0;

    if (configuration) {

      char key[7];

      snprintf(key, 7, "FANIN%X", i);

      name = OSDynamicCast(OSString, configuration->getObject(key));
    }

    UInt64 nameLength = name ? name->getLength() : 0;

    if (readTachometer(i) > minFanRPM || nameLength > 0) {
      if (!addTachometer(i, (nameLength > 0 ? name->getCStringNoCopy() : 0))) {
        WarningLog("ERROR adding tachometer sensor %d", i);
      }
    }
  }

  // Temperatures
  for (int i = 0; i < 2; i++) {

    if (configuration) {

      char key[8];

      snprintf(key, 8, "TEMPIN%X", i);

      OSString  *name = configuration ? OSDynamicCast(OSString, configuration->getObject(key)) : 0;

      if ((name && name->isEqualTo("System")) || (!configuration && i==0)) {
        if (!addSensor(KEY_NORTHBRIDGE_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor, i)) {
          WarningLog("ERROR adding System temperature sensor");
        }
      }
      else if ((name && name->isEqualTo("Processor")) || (!configuration && i==1)) {
        if (!addSensor(KEY_CPU_HEATSINK_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor,i)) {
          WarningLog("ERROR adding Processor temperature sensor");
        }
      }
    }
  }

  // Voltages
  for (int i = 0; i < 9; i++) {

    char key[5];

    snprintf(key, 5, "VIN%X", i);

    OSString  *name = configuration ? OSDynamicCast(OSString, configuration->getObject(key)) : 0;

    if ((name && name->isEqualTo("Processor")) || (!configuration && i==0)) {
      if (!addSensor(KEY_CPU_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding Processor Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("+12V")) || (!configuration && i==1)) {
      if (!addSensor(KEY_12V_VOLTAGE, TYPE_FP2E/*TYPE_FP4C*/, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding +12 Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("AVCC")) || (!configuration && i==2)) {
      if (!addSensor(KEY_AVCC_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding AVCC Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("3VCC")) || (!configuration && i==3)) {
      if (!addSensor(KEY_3VCC_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding 3VCC Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("+5V")) || (!configuration && i==4)) {
      if (!addSensor(KEY_CPU_VRM_SUPPLY0, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding +5V Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("5VSB")) || (!configuration && i==5)) {
      if (!addSensor(KEY_CPU_VRM_SUPPLY1, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding 5VSB Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("+3.3V")) || (!configuration && i==6)) {
      if (!addSensor(KEY_CPU_VRM_SUPPLY2, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding +3.3V Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("3VSB")) || (!configuration && i==7)) {
      if (!addSensor(KEY_3VSB_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding 3VSB Voltage Sensor!");
      }
    }
    else if ((name && name->isEqualTo("VBAT")) || (!configuration && i==8)) {
      if (!addSensor(KEY_VBAT_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i)) {
        WarningLog("ERROR Adding VBAT Voltage Sensor!");
      }
    }
  }

  return true;
}

const char *NCT677x::getModelName()
{
  switch (model) {
    case NCT6771F:  return "NCT6771F";
    case NCT6776F:  return "NCT6776F";
  }

  return "Unknown";
}