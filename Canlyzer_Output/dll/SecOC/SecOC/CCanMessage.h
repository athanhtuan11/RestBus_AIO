#ifndef CCanMessage_H_
#define CCanMessage_H_
/*!
********************************************************************************
@class CCanMessage
@ingroup can
@brief Base class for a CAN message

@author Markus Oenning, om72si
@author Robert Erhart, ett2si (10.01.2007)
@author Xenolation framework is part of CLARA
@copyright (c) Copyright Robert Bosch GmbH 2007 All rights reserved.
********************************************************************************
@remark
********************************************************************************
@todo add TimeoutCounter for send messages
********************************************************************************
*/

#include <stdint.h> //include uint8_t
#include <vector>

#include <xenolation/frameworkBus/CSignal.h>
#include <xenolation/frameworkCore/CClaraItemClassBase.h>
#include <xenolation/frameworkBus/CBasicPDU.h>
#include <xenolation/frameworkCore/CTime.h>

#include "CCanDrv.h"

////*********************************
//// CCanMessage
////*********************************
class CCanMessage : public CClaraItemClassBase<CCanMessage>, public CTime
{
    //*********************************
    // constructor/destructor/copy/move
    //*********************************
public:
    CCanMessage();
    virtual ~CCanMessage();

    //*******************************
    //classes
    //*******************************

    //*******************************
    // methods
    //*******************************
public:
    //! used only in CanController Task
    void init();

    //! used only in CanController Task
    //! @param[in] f_dT_r [ns] delta time between last call
    //! @param[out] f_message_r [CANmessage_t] can_message struct from CanRt
    //! @param[in] f_groupBitmask [bitmask] groups to be received
    //! @return status 0=OK ; -1=ERROR
    void recvCycle( const uint64_t& f_dT_r, const CANmessage_t& f_message_r, uint64_t f_groupBitmask = 0xffffffffffffffffULL );

    //! used only in CanController Task
    //! @param[in] f_dT_r [s] delta time between last call
    //! @param[in] f_message_r [struct] can_message struct for CanRt
    //! @param[in] f_groupBitmask [bitmask] groups to be send
    //! @return [bool] true: send; false: dont't send
    bool sendCycle( const uint64_t& f_dT_r, CANmessage_t& f_message_r, uint64_t f_groupBitmask = 0xffffffffffffffffULL );

    //! get Data Length Code DLC from message (Attention: for CAN-FD dlc is real data length, not code!)
    //! @return [Bytes] Data Length Code 0-8
    uint8_t getDLC();

    //! get Data Length Code DLC from message (Attention: for CAN-FD dlc is real data length, not code!)
    //! @return [Bytes] Data Length Code 0-8 of received Message
    uint8_t getDLCActual();

    //! get CanId from message
    //! @return [int] CAN ID from message
    uint32_t getCanId();

    //!  set a group to which the message belongs. Used by the send and recv method to decide if the message should be considered.
    //!  Message only be considered, if group belongs to the bitmask parameter of recvCycle or sendCyle method.
    //!  DEFAULT: 1
    //! @param[in] f_group [bitmask] cycle time of the CAN message
    void setGroup( uint64_t f_group );

    //!  set cycle time and Watch Dog (+-10% cycle time)
    //! @param[in] f_cycleTime [s] cycle time of the CAN message
    void setCycleTime( long double f_cycleTime ); //set also Cycle Watch Dog

    //! send messages: get the delta time for the last message and next message\n
    //! recv messages: get the delta time between the two last receive messages
    //! @return [s] actual cycle time
    long double getActualCycleTime();

    //! get desired cycle time for messages (set by setCycleTime)
    //! @return [s] desired cycle time
    long double getCycleTime();

    //! get desired cycle time for messages (set by setCycleTime)
    //! @param[in] f_lowerBoundary [s] lower boundary for watch dog
    //! @param[in] f_upperBoundary [s] cycle time of the CAN message
    void setCycleWatchDog( long double f_lowerBoundary, long double f_upperBoundary );

    //! send message: set time between last send message and next send message
    //! recv message: don't use
    //! @param[in] f_timeout [s] timeout between two messages
    void setTimeout( long double f_timeout );

    //! send message: set new/error DLC code for n cycles, if setDlcErrorCycles( n ) is called
    //! recv message: don't use
    //! @param[in] f_dlcError [bytes] new dlc code (Attention: for CAN-FD dlc is real data length, not code!)
    void setDlcError( uint8_t f_dlcError );

    //! send message: set new/error DLC code (set from method setDlcError) for f_dlcError cycles
    //! recv message: don't use
    //! @param[in] f_dlcErrorCycles [int] number of error cycles
    void setDlcErrorCycles( uint32_t f_dlcErrorCycles );

    //! send message: get remaining DLC error cycles
    //! recv message: get DLC error counter
    //! @return [int] actual DLC error counter
    uint32_t getDlcErrorCycles();

    //! send/recv message: get DLC error cycles with history
    //! @return [int] occurring DLC error counter
    uint32_t getDlcErrorCyclesOccurence();

    //! reset occurring DLC error counter
    //! @param[in] dummy [dummy] not used, only for MA Interface
    void resetDlcErrorCyclesOccurrence( bool dummy = true );

    //! send message: none
    //! recv message: get timeout counter
    //! @return [uint32] timeout counter
    uint32_t getCycleTimeoutCounter();

    //! send message: get remaining timeout time
    //! recv message: none
    //! @return [long double] remaining timeout time
    long double getRemainingTimeout();

    //! send message: none
    //! recv message: get timeout counter with history
    //! @return [uint32] occurring timeout counter
    uint32_t getCycleTimeoutCounterOccurence();

    //! send message: get cycle counter
    //! recv message: get cycle counter
    //! @return [uint64] counter
    uint64_t getCycleCounter();

    //! send message: set cycle counter
    //! recv message: set cycle counter
    //! @param[in] f_counter [uint64_t] counter
    void setCycleCounter( uint64_t f_counter );

    //! reset occurring Timeout counter
    //! @param[in] dummy [dummy] not used, only for MA Interface
    void resetCycleTimeoutCounterOccurrence( bool dummy = true );

    //! send message: none
    //! recv message: get cycle watchdog counter
    //! @return [uint32] cycle watchdog counter
    uint32_t getCycleErrorCounter();

    //! send message: none
    //! recv message: get cycle watchdog counter with history
    //! @return [uint32] occurring cycle watchdog counter
    uint32_t getCycleErrorCounterOccurence();

    //! reset occurring cycle error counter
    //! @param[in] dummy [dummy] not used, only for MA Interface
    void resetCycleErrorCounterOccurrence( bool dummy = true );

    //! reset the ring buffers of the containing signals
    //! @param[in] dummy [dummy] not used, only for MA Interface
    void signalTraceReset( bool dummy = true );

    //! reset the error stimulations of the containing signals and message
    //! @param[in] dummy [dummy] not used, only for MA Interface
    void stimCyclesReset( bool dummy = true );

    //! send message: none
    //! recv message: timestamp of the last received message
    //! @return [ns] xenomai timestamp of the message
    uint64_t getTimeStampNs();

    //! measurement and calibration configuration for claraServer
    //! @param[in] f_label Prefix of signal label
    virtual void buildCaliMeasVectors( ::std::string f_label )
    {
        appendMeasSingleItem( f_label, ".getDLC", &CCanMessage::getDLC );
        appendMeasSingleItem( f_label, ".getDLCActual", &CCanMessage::getDLCActual );
        appendMeasSingleItem( f_label, ".getCanId", &CCanMessage::getCanId );
        appendCaliCycleItem( f_label, ".setCycleTime", &CCanMessage::setCycleTime );
        appendMeasSingleItem( f_label, ".getActualCycleTime", &CCanMessage::getActualCycleTime );
        appendMeasSingleItem( f_label, ".getCycleTime", &CCanMessage::getCycleTime );
        appendCaliCycleItem( f_label, ".setTimeout", &CCanMessage::setTimeout );
        appendCaliCycleItem( f_label, ".setDlcError", &CCanMessage::setDlcError );
        appendCaliCycleItem( f_label, ".setDlcErrorCycles", &CCanMessage::setDlcErrorCycles );
        appendMeasSingleItem( f_label, ".getDlcErrorCycles", &CCanMessage::getDlcErrorCycles );
        appendMeasSingleItem( f_label, ".getDlcErrorCyclesOccurence", &CCanMessage::getDlcErrorCyclesOccurence );
        appendCaliCycleItem( f_label, ".resetDlcErrorCyclesOccurrence", &CCanMessage::resetDlcErrorCyclesOccurrence );
        appendMeasSingleItem( f_label, ".getCycleTimeoutCounter", &CCanMessage::getCycleTimeoutCounter );
        appendMeasSingleItem( f_label, ".getRemainingTimeout", &CCanMessage::getRemainingTimeout );
        appendMeasSingleItem( f_label, ".getCycleCounter", &CCanMessage::getCycleCounter );
        appendCaliCycleItem( f_label, ".setCycleCounter", &CCanMessage::setCycleCounter );
        appendMeasSingleItem( f_label, ".getCycleTimeoutCounterOccurence", &CCanMessage::getCycleTimeoutCounterOccurence );
        appendCaliCycleItem( f_label, ".resetCycleTimeoutCounterOccurrence", &CCanMessage::resetCycleTimeoutCounterOccurrence );
        appendMeasSingleItem( f_label, ".getCycleErrorCounter", &CCanMessage::getCycleErrorCounter );
        appendMeasSingleItem( f_label, ".getCycleErrorCounterOccurence", &CCanMessage::getCycleErrorCounterOccurence );
        appendCaliCycleItem( f_label, ".resetCycleErrorCounterOccurrence", &CCanMessage::resetCycleErrorCounterOccurrence );
        appendCaliCycleItem( f_label, ".signalTraceReset", &CCanMessage::signalTraceReset );
        appendCaliCycleItem( f_label, ".stimCyclesReset", &CCanMessage::stimCyclesReset );
        //appendMeasSingleItem( f_label, ".getTimeStampNs", &CCanMessage::getTimeStampNs );
    };

protected:
    //! set desired DLC code of the message (Attention: for CAN-FD dlc is real data length, not code!)
    //! @param[in] f_dlc [bytes] number of bytes
    void setDlc( uint8_t f_dlc );

    //! set CAN ID of the message
    //! @param[in] f_canId [uint] CAN ID extended or standard
    void setCanId( uint32_t f_canId );

    //! set CAN Message Type of the tx message
    //! @param[in] f_canFDMessageType [uint] CAN Frame type
    //! @remark
    //!    PCANFD_MSG_STD      0x00000000
    //!    PCANFD_MSG_EXT      0x00000002
    //!    PCANFD_MSG_BRS      0x00100000 : BitRateSwitch nominal -> data rate
    //!    examples to use
    //!    setCanFDMessageType(PCANFD_MSG_STD)
    //!    setCanFDMessageType(PCANFD_MSG_EXT)
    //!    setCanFDMessageType(PCANFD_MSG_EXT|PCANFD_MSG_BRS)  extended ID and BRS frame
    void setCanFDMessageType( uint32_t f_canFDMessageType );


    //! set CAN FD Type of the message
    //! @param[in] f_canId [uint16_t]:  PCANFD_TYPE_CAN20_MSG    1
    //!                                 PCANFD_TYPE_CANFD_MSG    2
    void setCanFDType( uint16_t f_canFDType );

    //! send message: set offset time for the first sending of the message. Simulate time slots on CAN
    //! recv message: dont' use
    //! @param[in] f_offset [s] offset time of CAN message
    void setOffset( long double f_offset );

    //! add signal and
    //! bind signal to message buffer and timestamp
    //! @param[in] f_signal_r [CSignalBase] signal class
    void addSignal( CSignalBase& f_signal_r );

    //! add PDU to PDU (PDU in PDU)
    //! @param[in] f_Pdu_r reference to PDU
    //! @param[in] f_PduType[enum] default=FIXED_e values:FIXED_e, AUTOSARPDU_e, CONTAINERPDU_e, SoAd_e, USER_e
    void addPdu( CBasicPDU& f_Pdu_r, BASICPDU::pdutype_t f_PduType = BASICPDU::FIXED_e );

    //! add PDU to PDU (PDU in PDU)
    //! @param[in] f_Pdu_r reference to PDU
    //! @param[in] f_PduOffset offset in Bytes to parent frame/PDU start
    //! @param[in] f_PduType[enum] default=FIXED_e values:FIXED_e, AUTOSARPDU_e, CONTAINERPDU_e, SoAd_e, USER_e
    void addPdu( CBasicPDU& f_Pdu_r, int f_PduOffset, BASICPDU::pdutype_t f_PduType = BASICPDU::FIXED_e );

    //! pure virtual method. Overload in message class.
    virtual void initMessage() = 0;

    //! pure virtual method. Overload in message class.
    virtual void initMessageFlexGen() = 0;

    //! pure virtual method. Overload in message class.
    virtual void initSignals() = 0;

    //! pure virtual method. Overload in message class.
    virtual void calcRecvSignals( uint64_t f_timeStamp ) = 0;

    //! pure virtual method. Overload in message class.
    virtual bool calcSendSignals( uint64_t f_timeStamp ) = 0;

private:
    void initPdu();

    //*******************************
    //variables
    //*******************************
public:
protected:
    uint8_t m_data_a[64 + 8]; // reason see CSignal (64bit) ????
    uint8_t* m_data_p;
    uint8_t** m_data_p2;
private:
    uint64_t m_lowerBoundary;
    uint64_t m_upperBoundary;
    uint32_t m_dlcErrorCycles;
    uint32_t m_dlcErrorCyclesOccurence;
    uint32_t m_cycleTimeoutCounter;
    uint32_t m_remainingTimeout;
    uint32_t m_cycleTimeoutCounterOccurence;
    uint64_t m_cycleCounter;
    uint32_t m_cycleErrorCounter;
    uint32_t m_cycleErrorCounterOccurence;
    int64_t m_cycleTimeNs;
    uint64_t m_cycleTimeActualNs;
    uint64_t m_offsetTimeNs;
    uint64_t m_timeoutNs;
    uint8_t m_dlc;
    uint8_t m_dlcActual;
    uint8_t m_dlcErr;
    uint32_t m_canId;
    uint32_t m_canFD_MsgType;
    uint16_t m_canFD_Type;
    uint64_t m_group;
    long double m_timeStamp;
    uint64_t m_timeStampNsK1;
    int64_t m_sendRecvTimerNs;
    ::std::vector<CSignalBase*> m_signal_a;
    ::std::vector<CBasicPDU*> m_pdu_a;
};

#endif /*CCanMessage_H_*/
