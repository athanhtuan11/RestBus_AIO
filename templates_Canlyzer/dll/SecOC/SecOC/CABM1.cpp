/*!
********************************************************************************
Source: CTX_canTemplate.cpp
Date: 2015/03/16 16:41:01CST
Revision: 1.1

author     Tim Schaechterle (tsh9fe)
author     (c) Copyright Robert Bosch GmbH 2007 All rights reserved.

********************************************************************************
*/

#include "CABM1.h"
#include "../CCan1Task.h"
extern CCan1Task CAN1;
#include <simulation/main.h>

//------------------------
// constructor/destructor
//------------------------
CABM1::CABM1()
{
	initMessageFlexGen();
}
CABM1::~CABM1()
{
}
//------------------------
// public methods
//------------------------

void CABM1::initMessageFlexGen()
{
	setDlc(16);
	setCanId(849UL);
	setCycleTime(0.5);
	setOffset(UNDEFINED);
	setCanFDMessageType(PCANFD_MSG_BRS);
	setCanFDType(PCANFD_TYPE_CANFD_MSG);
}
void CABM1::initMessage()
{
}
//------------------------
// private methods
//------------------------

void CABM1::initSignals()
{
	//add_signal
	addSignal(CheckSum_ABM1.init(1, 0, 7, 8));
	addSignal(PABSwtSts.init(1, 0, 15, 1));
	addSignal(SecRowLSBR.init(1, 0, 14, 1));
	addSignal(AirbFailLmpCmd.init(1, 0, 13, 1));
	addSignal(SecRowMidSBR.init(1, 0, 12, 1));
	addSignal(DrvSBR.init(1, 0, 11, 1));
	addSignal(SecRowRSBR.init(1, 0, 10, 1));
	addSignal(PassSBR.init(1, 0, 9, 1));
	addSignal(CrashOutputSts.init(1, 0, 23, 8));
	addSignal(SecRowMidSBR_Visual.init(1, 0, 31, 2));
	addSignal(SecRowLSBR_Visual.init(1, 0, 29, 2));
	addSignal(PassSBR_Visual.init(1, 0, 27, 2));
	addSignal(DrvSBR_Visual.init(1, 0, 25, 2));
	addSignal(SecRowRSBR_Visual.init(1, 0, 33, 2));
	addSignal(RollingCounter_ABM1.init(1, 0, 59, 4));
	addSignal(FreshnessValue_ABM1.init(1, 0, 71, 16));
	addSignal(MAC_Check_ABM1.init(1, 0, 87, 48));
	m_ResCNT = MasterCenterClock.m_Resetcount;

}
void CABM1::calcRecvSignals(uint64_t f_timeStamp)
{
	//add_RecvSignals_writePhysValue

}
bool CABM1::calcSendSignals(uint64_t f_timeStamp)
{
	//



	if (MasterCenterClock.m_Resetcount > m_ResCNT) //if the current ResetCnt is bigger than local ResCnt, it means ResetCnt already increased. MesgCnt need to reset
	{

		m_count = 0;

	}
	m_ResCNT = MasterCenterClock.m_Resetcount;//	After determine if the MesgCnt m_count needed to reset to 1. Then Sync the current ResetCnt with Local ResetCnt at each Message cycle

											  //add_SendSignals_writePhysValue
											  // CheckSum_ABM1.writePhysValue( UNDEFINED );
	PABSwtSts.writePhysValue(UNDEFINED);
	SecRowLSBR.writePhysValue(UNDEFINED);
	AirbFailLmpCmd.writePhysValue(UNDEFINED);
	SecRowMidSBR.writePhysValue(UNDEFINED);
	DrvSBR.writePhysValue(UNDEFINED);
	SecRowRSBR.writePhysValue(UNDEFINED);
	PassSBR.writePhysValue(UNDEFINED);
	CrashOutputSts.writePhysValue(UNDEFINED);
	SecRowMidSBR_Visual.writePhysValue(UNDEFINED);
	SecRowLSBR_Visual.writePhysValue(UNDEFINED);
	PassSBR_Visual.writePhysValue(UNDEFINED);
	DrvSBR_Visual.writePhysValue(UNDEFINED);
	SecRowRSBR_Visual.writePhysValue(UNDEFINED);
	RollingCounter_ABM1.writeCounter(0, 14, 1);

	uint8_t t_index;
	uint8_t DataPtr[64];
	uint8_t CheckSum;
	DataPtr[0] = 0x3a;
	DataPtr[1] = 0x00;
	CheckSum = PROJ_CHK_SAE_J1850_0x1D(DataPtr, 2, 0xFF, 1);

	for (t_index = 1; t_index < 8; t_index++)
	{
		DataPtr[t_index - 1] = m_data_a[t_index];
	}
	CheckSum = PROJ_CHK_SAE_J1850_0x1D(DataPtr, 7, CheckSum, 0);
	CheckSum ^= 0xFF;
	CheckSum_ABM1.writePhysValue(CheckSum);

	FreshnessValue_ABM1.writeRawValue(GenerateCompleteFreshValue(m_count, MasterCenterClock.m_TripCount, MasterCenterClock.m_Resetcount, MasterCenterClock.m_Resetcount & 0x3) & 0xFFFF);// TFV is 2 bytes
	MAC_Check_ABM1.writeRawValue(calcAuth(*m_data_p2, m_key, m_len, 6, 0x0351, GenerateCompleteFreshValue(m_count, MasterCenterClock.m_TripCount, MasterCenterClock.m_Resetcount, MasterCenterClock.m_Resetcount & 0x3)));

	m_count++;

	return true;
}
