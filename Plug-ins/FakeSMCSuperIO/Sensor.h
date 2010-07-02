/*
 *  Sensor.h
 *  FakeSMCSuperIO
 *
 *  Created by mozo on 28/06/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#ifndef _SENSOR_H 
#define _SENSOR_H

#include "Binding.h"

class Sensor : public Binding 
{
protected:
	SuperIO*	m_Provider;
	UInt8		m_Index;
public:
	Sensor(SuperIO* provider, UInt8 index, const char* key, const char* type, UInt8 size) : Binding(key, type, size)
	{
		m_Provider = provider;
		m_Index = index;
	};
	
	UInt8 GetIndex() { return m_Index; };
};

#endif