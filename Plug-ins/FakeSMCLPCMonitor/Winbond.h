/*
 *  Winbond.h
 *  FakeSMCLPCMonitor
 *
 *  Created by Mozodojo on 29/05/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#ifndef WINBOND_H
#define WINBOND_H 

#include "SuperIO.h"

const UInt8 WINBOND_HARDWARE_MONITOR_LDN	= 0x0B;

const UInt16 WINBOND_VENDOR_ID = 0x5CA3;
const UInt8 WINBOND_HIGH_BYTE = 0x80;

// Winbond Hardware Monitor
const UInt8 WINBOND_ADDRESS_REGISTER_OFFSET = 0x05;
const UInt8 WINBOND_DATA_REGISTER_OFFSET = 0x06;

// Winbond Hardware Monitor Registers
const UInt8 WINBOND_VOLTAGE_BASE_REG = 0x20;
const UInt8 WINBOND_BANK_SELECT_REGISTER = 0x4E;
//const UInt8 WINBOND_VENDOR_ID_REGISTER = 0x4F;
const UInt8 WINBOND_TEMPERATURE_SOURCE_SELECT_REG = 0x49;

//private string[] TEMPERATURE_NAME = 
//new string[] {"CPU", "Auxiliary", "System"};
const UInt8 WINBOND_TEMPERATURE_REG[] = { 0x50, 0x50, 0x27 };
const UInt8 WINBOND_TEMPERATURE_BANK[] = { 1, 2, 0 };

const UInt8 WINBOND_FAN_TACHO_REG[] = { 0x28, 0x29, 0x2A, 0x3F, 0x53 };
const UInt8 WINBOND_FAN_TACHO_BANK[] = { 0, 0, 0, 0, 5 };       
const UInt8 WINBOND_FAN_BIT_REG[] = { 0x47, 0x4B, 0x4C, 0x59, 0x5D };
const UInt8 WINBOND_FAN_DIV_BIT0[] = { 36, 38, 30, 8, 10 };
const UInt8 WINBOND_FAN_DIV_BIT1[] = { 37, 39, 31, 9, 11 };
const UInt8 WINBOND_FAN_DIV_BIT2[] = { 5, 6, 7, 23, 15 };

class Winbond : public SuperIO
{
private:
protected:
	UInt16	LastVcore;
	
	UInt8	FanOffset;
	UInt8	FanCount;
	UInt8	FanIndex[5];
	UInt16	FanValue[5];
	bool	FanValueObsolete[5];
	
	UInt8	ReadByte(UInt8 bank, UInt8 reg);
	void	WriteByte(UInt8 bank, UInt8 reg, UInt8 value);
	short	ReadTemperature(UInt8 index);
	UInt64	SetBit(UInt64 target, UInt32 bit, UInt32 value);
	void	UpdateRPM();
	void	Enter();
	void	Exit();
public:	
	virtual bool	Probe();
	virtual void	Init();
	virtual void	Finish();
	
	void Update(const char* key, char* data);
};

#endif