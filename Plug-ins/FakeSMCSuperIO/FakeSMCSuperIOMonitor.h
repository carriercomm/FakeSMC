/*
 *
 *  Copyright © 2010 mozodojo. All rights reserved.
 *
 */

#ifndef FakeSMCLPCMonitor_h
#define FakeSMCLPCMonitor_h

#include <IOKit/IOService.h>

#include "ITE.h"
#include "Winbond.h"
#include "Fintek.h"
#include "SMSC.h"

class FakeSMCSuperIOMonitor : public IOService
{
    OSDeclareDefaultStructors(FakeSMCSuperIOMonitor)    
private:

protected:
	SuperIO*			superio;
public:	
	virtual bool		init(OSDictionary *properties=0);
	virtual IOService*	probe(IOService *provider, SInt32 *score);
    virtual bool		start(IOService *provider);
	virtual void		stop(IOService *provider);
	virtual void		free(void);
};

#endif
