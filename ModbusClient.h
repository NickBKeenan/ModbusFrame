/**
@file
Arduino library for communicating with Modbus slaves over RS232/485 (via RTU protocol).

@defgroup setup ModbusMaster Object Instantiation/Initialization
@defgroup buffer ModbusMaster Buffer Management
@defgroup discrete Modbus Function Codes for Discrete Coils/Inputs
@defgroup register Modbus Function Codes for Holding/Input Registers
@defgroup constant Modbus Function Codes, Exception Codes
*/
/*

  ModbusMaster.h - Arduino library for communicating with Modbus slaves
  over RS232/485 (via RTU protocol).

  Library:: ModbusMaster

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/

  
#ifndef ModbusClient_h
#define ModbusClient_h
#include "ModbusFrame.h"


/* _____STANDARD INCLUDES____________________________________________________ */
// include types & constants of Wiring core API
#include "Arduino.h"

/* _____UTILITY MACROS_______________________________________________________ */


/* _____PROJECT INCLUDES_____________________________________________________ */
// functions to calculate Modbus Application Data Unit CRC
#include "crc16.h"

// functions to manipulate words
#include "word.h"


/* _____CLASS DEFINITIONS____________________________________________________ */
/**
Arduino class library for communicating with Modbus servers over 
RS232/485 (via RTU protocol).
*/
class ModbusClient: public ModbusFrame
{
    uint8_t _u8MBServerID;
  public:
    
   
    void begin(uint8_t ServerID, Stream &serial);
    

    
    
    
    
    
    
    
    
    uint8_t  readHoldingRegisters(uint16_t, uint16_t); //
    uint8_t  readInputRegisters(uint16_t, uint8_t);  //
    uint8_t  writeMultipleRegisters(uint16_t, uint16_t);//

    void DoServerAction();
    
    
    // master function that conducts Modbus transactions
protected:
    uint8_t ModbusClientTransaction(uint8_t u8MBFunction,  uint16_t u16ReadQty, uint16_t Address);

    
    
};
#endif

/**
@example examples/Basic/Basic.pde
@example examples/PhoenixContact_nanoLC/PhoenixContact_nanoLC.pde
@example examples/RS485_HalfDuplex/RS485_HalfDuplex.ino
*/
