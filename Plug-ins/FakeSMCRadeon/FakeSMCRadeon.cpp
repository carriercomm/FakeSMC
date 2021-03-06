/* 
 *  FakeSMC ATI Radeon GPU monitoring plugin
 *  Created by Slice 23.07.2010
 *  Copyright 2010 Slice. All rights reserved.
 *
 */
#include "FakeSMCRadeonMon.h"

#define kTimeoutMSecs 1000
#define fVendor "vendor-id"
#define fDevice "device-id"

//#define kIOPCIConfigBaseAddress0 0x10

#undef super
#define super IOService

OSDefineMetaClassAndStructors(RadeonPlugin, IOService) 

//static int count=0;

IOService*
RadeonPlugin::probe(IOService *provider, SInt32 *score)
{
	
	OSData*		prop;
	
	prop = OSDynamicCast( OSData , provider->getProperty(fVendor)); // safe way to get vendor
	if(prop)
	{
		UInt32 vendorID = *(UInt32*) prop->getBytesNoCopy();
		if( (vendorID & 0xffff) != 0x1002) //check if vendorID is really ATI, if not don't bother
		{
			//IOLog("FakeSMC_Radeon: Can't Find ATI Chip!\n");
			//IOLog("FakeSMC_Radeon: Test by Intel!\n");  //Debug
			return( 0 );
		}		
	}
	prop = OSDynamicCast( OSData , provider->getProperty(fDevice)); // safe way to get device
	if(prop)
	{
		deviceID = *(UInt32*) prop->getBytesNoCopy();
		IOLog("FakeSMC_Radeon: found %04lx chip\n", (long unsigned int)deviceID & 0xffff);
	}
	
	
    if( !super::probe( provider, score ))
		return( 0 );
	//	IOLog("FakeSMC_Radeon: probe success\n");	
	return (this);
}

bool
RadeonPlugin::start( IOService * provider ) {
	if(!provider || !super::start(provider))
		return false;
	Card = new ATICard();
	Card->VCard = (IOPCIDevice*)provider;
	Card->chipID = deviceID;	
	return Card->initialize();	
	
}

bool RadeonPlugin::init(OSDictionary *properties)
{    
	return super::init(properties);
}

void RadeonPlugin::stop (IOService* provider)
{
	UpdateFNum();
	super::stop(provider);
}

void RadeonPlugin::free ()
{
	if (Card->rinfo) {
		IOFree(Card->rinfo, sizeof(RADEONCardInfo));
	}
	if (Card) {
		delete Card;
	}
	
	super::free ();
}

