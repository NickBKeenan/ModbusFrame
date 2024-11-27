/**
@file
Arduino library for communicating with Modbus Servers over RS232/485 (via RTU protocol).
*/
/*

  ModbusMaster.cpp - Arduino library for communicating with Modbus Servers
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


/* _____PROJECT INCLUDES_____________________________________________________ */
#include "ModbusClient.h"


/* _____GLOBAL VARIABLES_____________________________________________________ */


/* _____PUBLIC FUNCTIONS_____________________________________________________ */

/**
Initialize class object.

Assigns the Modbus Server ID and serial port.
Call once class has been instantiated, typically within setup().

@param Server Modbus Server ID (1..255)
@param &serial reference to serial port object (Serial, Serial1, ... Serial3)
@ingroup setup
*/
void ModbusClient::begin(uint8_t ServerID, Stream &serial)
{

  _u8MBServerID = ServerID;
  
  ModbusFrame::begin(serial);
  

}

























/**
Modbus function 0x03 Read Holding Registers.

This function code is used to read the contents of a contiguous block of 
holding registers in a remote device. The request specifies the starting 
register address and the number of registers. Registers are addressed 
starting at zero.

The register data in the response buffer is packed as one word per 
register.

@param u16ReadAddress address of the first holding register (0x0000..0xFFFF)
@param u16ReadQty quantity of holding registers to read (1..125, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusClient::readHoldingRegisters(uint16_t u16ReadAddress,
  uint16_t u16ReadQty)
{
  return ModbusClientTransaction(MODBUS03_ReadHoldingRegisters, u16ReadQty, u16ReadAddress);
  
}


/**
Modbus function 0x04 Read Input Registers.

This function code is used to read from 1 to 125 contiguous input 
registers in a remote device. The request specifies the starting 
register address and the number of registers. Registers are addressed 
starting at zero.

The register data in the response buffer is packed as one word per 
register.

@param u16ReadAddress address of the first input register (0x0000..0xFFFF)
@param u16ReadQty quantity of input registers to read (1..125, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusClient::readInputRegisters(uint16_t u16ReadAddress,
  uint8_t u16ReadQty)
{
  return ModbusClientTransaction(MODBUS04_ReadInputRegisters, u16ReadQty, u16ReadAddress);
  
}




/**
Modbus function 0x10 Write Multiple Registers.

This function code is used to write a block of contiguous registers (1 
to 123 registers) in a remote device.

The requested written values are specified in the transmit buffer. Data 
is packed as one word per register.

@param u16WriteAddress address of the holding register (0x0000..0xFFFF)
@param u16WriteQty quantity of holding registers to write (1..123, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusClient::writeMultipleRegisters(uint16_t u16WriteAddress,
  uint16_t u16WriteQty)
{
  return ModbusClientTransaction(MODBUS10_WriteMultipleRegisters, u16WriteQty, u16WriteAddress);
}







/* _____PRIVATE FUNCTIONS____________________________________________________ */
/**
Modbus transaction engine.
Sequence:
  - assemble Modbus Request Application Data Unit (ADU),
    based on particular function called
  - transmit request over selected serial port
  - wait for/retrieve response
  - evaluate/disassemble response
  - return status (success/exception)

@param u8MBFunction Modbus function (0x01..0xFF)
@return 0 on success; exception number on failure
*/
uint8_t ModbusClient::ModbusClientTransaction(uint8_t u8MBFunction,  uint16_t Quantity, uint16_t Address)
{
    SendFrame(u8MBFunction, _u8MBServerID, Quantity, Address, true);

    uint8_t u8MBStatus=0; 
    u8MBStatus=ReceiveFrame( _u8MBServerID, true);
  
    return u8MBStatus;

}
