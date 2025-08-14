/*!
********************************************************************************
Source: CTX_canTemplate.cpp
Date: 2015/03/16 16:41:01CST
Revision: 1.1

author     Tim Schaechterle (tsh9fe)
author     (c) Copyright Robert Bosch GmbH 2007 All rights reserved.
********************************************************************************
*/

#ifndef CABM1_H_
#define CABM1_H_

#include <xenolation/frameworkBus/can/CCanMessage.h>
#include "../../CSimModel.h"
#include "../../GFunctionLib.h"
extern CSimModel SimModel;
#include "../../../Security/aes.h"
//#include "CPDUCentralClock_SECURED.h"

// Message Description:
class CABM1 : public CCanMessage
{
public:
	// constructor/destructor
	CABM1();
	virtual ~CABM1();

	//flexgen_instances
	//<flexgen>
	//add_Csignal
	CSignal <long double, unsigned long long, 1> CheckSum_ABM1;
	CSignal <long double, unsigned long long, 1> PABSwtSts;
	CSignal <long double, unsigned long long, 1> SecRowLSBR;
	CSignal <long double, unsigned long long, 1> AirbFailLmpCmd;
	CSignal <long double, unsigned long long, 1> SecRowMidSBR;
	CSignal <long double, unsigned long long, 1> DrvSBR;
	CSignal <long double, unsigned long long, 1> SecRowRSBR;
	CSignal <long double, unsigned long long, 1> PassSBR;
	CSignal <long double, unsigned long long, 1> CrashOutputSts;
	CSignal <long double, unsigned long long, 1> SecRowMidSBR_Visual;
	CSignal <long double, unsigned long long, 1> SecRowLSBR_Visual;
	CSignal <long double, unsigned long long, 1> PassSBR_Visual;
	CSignal <long double, unsigned long long, 1> DrvSBR_Visual;
	CSignal <long double, unsigned long long, 1> SecRowRSBR_Visual;
	CSignal <long double, unsigned long long, 1> RollingCounter_ABM1;
	CSignal <long double, unsigned long long, 1> FreshnessValue_ABM1;
	CSignal <long double, unsigned long long, 1> MAC_Check_ABM1;
	uint8_t m_key[16] = { 0x04,0x08,0x2E,0x14,0x58,0x6C,0x06,0xEE,0x56,0x0E,0x0E,0x2E,0x0C,0x02,0x7E,0x62 }; // PDU specific key to use with AES algorithm
	uint16_t m_len = 16; // pdu length including the four 8 bytes
	uint32_t m_count = 0;
	uint16_t m_ResCNT = 0;

	//</flexgen>

private:
	// methods
	void initMessageFlexGen();
	void initMessage();

	void initSignals();
	void calcRecvSignals(uint64_t f_timeStamp);
	bool calcSendSignals(uint64_t f_timeStamp);
	// variables

};
#endif
