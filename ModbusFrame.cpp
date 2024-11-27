#include "ModbusFrame.h"


/* _____GLOBAL VARIABLES_____________________________________________________ */


/* _____PUBLIC FUNCTIONS_____________________________________________________ */

/**
Initialize class object.

Assigns the Modbus slave ID and serial port.
Call once class has been instantiated, typically within setup().

@param slave Modbus slave ID (1..255)
@param &serial reference to serial port object (Serial, Serial1, ... Serial3)
@ingroup setup
*/
void ModbusFrame::begin( Stream &serial)
{
  _serial = &serial;
  _u8TransmitBufferIndex = 0;
  u16TransmitBufferLength = 0;
  

}



























uint8_t ModbusFrame::SendFrame(uint8_t u8MBFunction, uint8_t u8MBSlave, uint16_t Quantity, uint16_t Address, bool isClient)
{

    uint8_t u8ModbusADU[256];
    uint8_t u8ModbusADUSize = 0;
    uint8_t i, u8Qty;
    uint16_t u16CRC;
   

    // assemble Modbus Request Application Data Unit
    u8ModbusADU[u8ModbusADUSize++] = u8MBSlave;
    u8ModbusADU[u8ModbusADUSize++] = u8MBFunction;
    if (isClient)
    {
        if (u8MBFunction== MODBUS04_ReadInputRegisters || u8MBFunction == MODBUS03_ReadHoldingRegisters)
        {
            u8ModbusADU[u8ModbusADUSize++] = highByte(Address);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(Address);
            u8ModbusADU[u8ModbusADUSize++] = highByte(Quantity);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(Quantity);
        
        }

        if (u8MBFunction== MODBUS10_WriteMultipleRegisters)
        {
        
            u8ModbusADU[u8ModbusADUSize++] = highByte(Address);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(Address);
            u8ModbusADU[u8ModbusADUSize++] = highByte(Quantity);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(Quantity);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(Quantity << 1);

            for (i = 0; i < lowByte(Quantity); i++)
            {
                u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[i]);
                u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[i]);
            }

        }

    }
    else // is server
    {
        

        //03 && 03
        if (u8MBFunction == MODBUS04_ReadInputRegisters || u8MBFunction == MODBUS03_ReadHoldingRegisters)
        {
            u8ModbusADU[u8ModbusADUSize++] = Quantity*2;

            int x;
            for (x = 0; x < Quantity; x++)
            {
                u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[x]);
                u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[x]);
            }
        }

        if (u8MBFunction == MODBUS10_WriteMultipleRegisters)
        {
            u8ModbusADU[u8ModbusADUSize++] = highByte(Address);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(Address);
            u8ModbusADU[u8ModbusADUSize++] = highByte(Quantity);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(Quantity);


        }
        
    }
    // append CRC
    u16CRC = 0xFFFF;
    for (i = 0; i < u8ModbusADUSize; i++)
    {
        u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
    }
    u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
    u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);
    u8ModbusADU[u8ModbusADUSize] = 0;

    // flush receive buffer before transmitting request
    while (_serial->read() != -1);
    //NK added the follwing three lines:
    digitalWrite(m_DE, HIGH);
    digitalWrite(m_RE, HIGH);
    delay(100);
    //end NK add
    // 
    // transmit request
    for (i = 0; i < u8ModbusADUSize; i++)
    {
        _serial->write(u8ModbusADU[i]);
    }

    u8ModbusADUSize = 0;
    _serial->flush();    // flush transmit buffer

      // NK added the following two lines
    digitalWrite(m_DE, LOW);
    digitalWrite(m_RE, LOW);
    // end NK add
    return 0;
}

/// <summary>
/// /////////////////////////////////////////////////////////////////////////////
/// </summary>
/// <param name="u8MBFunction"></param>
/// <returns></returns>
/// 
/// The first byte of a Modbus frame is the server address. Second byte is the function code. If the function takes multiple bytes, 
/// the third byte is the length of the data and bytes 4 and on are the data. Otherwise byte 3 is the data.
/// The final two bytes are the CRC checksum
/// Minimum frame size is five: address, function, data and two CRC. 
/// 
/// When parsing a stream, you have to prepare for three contingencies: 
/// 1. a well-formed packet for this server
/// 2. a well-formed packet, but not for this server (ignore)
/// 3. An incomplete packet -- wait for rest to arrive
/// 4. a poorly-formed packet --discard first byte and try parsing the rest of the stream
/// 
/// If fewer than five bytes are in the stream, it's incomplete, just deal with it later
/// If five or more bytes are in the stream, you might have a packet. You need to look at bytes 2 and 3 to see how many bytes this packet needs
/// If there aren't enough bytes wait until there are or the timeout expires. If the timeout expires, assume the packet is invalid
/// If there are enough bytes, look at them and see if the packet is valid
/// If the packet is adjudged to be invalid, discard the first byte in the stream and start over. 
/// If the packet is valid, remove it from the stream. Then assess whether it's meant for this server.
/// 
/// 
uint8_t ModbusFrame::ReceiveFrame( uint8_t ThisAddress, bool isClient)
{
  
    uint8_t i;
    uint32_t u32StartTime;
    int bytesneeded = 8;
    uint8_t u8MBStatus = ku8MBSuccess;
    uint8_t u8ModbusADU[256];
    uint8_t u8ModbusADUSize = 0;
    uint16_t u16CRC;

    u32StartTime = millis();

    // the presumption is that a client may call immediately upon executing a request and needs to wait for a response, which will only be to this client
    // a server has to check available() before calling and will skip over this. 
    while (!_serial->available())
    {
        if ((millis() - u32StartTime) > ku16MBResponseTimeout)
        {
            return ku8MBResponseTimedOut;
        }

    }

    // loop until we run out of time or bytes, or an error occurs

    bool done = false;
    bool newpacket = true;
    // the first byte of a packet is the target ID
    // ignore all bytes until you hit one that is the target ID
    bool haveHeader = false;
  while (!done)
  {
    if (_serial->available() )
    {
        if (!haveHeader)
        {
            uint8_t ch;
            ch = _serial->read();
            if (ch == ThisAddress)
            {
                haveHeader = true;
                u8ModbusADUSize = 1;
                u8ModbusADU[0] = ch;

            }
            else
            {
                // just discard the byte if it's not the header byte

            }
            
        }
        else
        {
            if (u8ModbusADUSize < 255)
            {
                uint8_t ch;
                ch = _serial->read();

                u8ModbusADU[u8ModbusADUSize++] = ch;


            }
        }
    }
    else
    {
        if (!haveHeader)
        {
            return ku8MBResponseTimedOut;
        }
    }
    
    // evaluate slave ID, function code once enough bytes have been read
    if (newpacket && u8ModbusADUSize >5)
    {
        LastReceivedAddress= u8ModbusADU[0];
        LastReceivedFunctionCode= u8ModbusADU[1];
        newpacket = false;
      // evaluate returned Modbus function code
        
        
          if (LastReceivedFunctionCode == MODBUS04_ReadInputRegisters || LastReceivedFunctionCode == MODBUS03_ReadHoldingRegisters)
          {
        
            if (isClient)  // if a client, we're getting the payload back
            {
                bytesneeded = u8ModbusADU[2] + 5;
            }
            else
            {
                // we're a server, there is no payload.
                bytesneeded = 8;
            }
          }
          
          
          if (LastReceivedFunctionCode == MODBUS10_WriteMultipleRegisters)
          {
              // need to put in client code;
              if (isClient)
              {
                  bytesneeded = 8;
              }
              else
              {
                  bytesneeded = u8ModbusADU[5]*2 + 9; ;
              }
          }
          
      }
    
    if ((millis() - u32StartTime) > ku16MBResponseTimeout)
    {
      u8MBStatus = ku8MBResponseTimedOut;
      Serial.println("Packet timed out!!");
      if (u8ModbusADUSize == 0)
          return u8MBStatus;
      else
          Serial.print(u8ModbusADUSize);
          

    }
    // check CRC
    if (u8ModbusADUSize ==bytesneeded)
    {
        // calculate CRC
        u16CRC = 0xFFFF;
        for (i = 0; i < (u8ModbusADUSize - 2); i++)
        {
            u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
        }

        // verify CRC
        if (!u8MBStatus && (lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2] ||
            highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1]))
        {
            u8MBStatus = ku8MBInvalidCRC;
            Serial.println("CRC Error");
            int x;
            for (x = 0; x < u8ModbusADUSize; x++)
            {
                Serial.print(u8ModbusADU[x], HEX);
                Serial.print(".");
            }
            Serial.println();

        }
        else
        {
            done = true;
            // we have a well-formed packet!
        }
    }

    if (u8MBStatus == ku8MBResponseTimedOut || u8MBStatus == ku8MBInvalidCRC)
    {
        // on these errors just discard the first byte and try again
        int x;
        haveHeader = false;
        int offset;
        for (x = 1; x < u8ModbusADUSize -1; x++)
        {
            if(haveHeader)
                u8ModbusADU[x-offset] = u8ModbusADU[x ];
            else
            {
                if (u8ModbusADU[x] == ThisAddress)
                {
                    haveHeader = true;
                    offset = x;
                    u8ModbusADU[0] = u8ModbusADU[x];
                }
            }
        }
        if (haveHeader)
            u8ModbusADUSize -= offset;
        else
            u8ModbusADUSize = 0;
        
        if ((u8ModbusADUSize < 5 && u8MBStatus == ku8MBResponseTimedOut))
        {
            done = true;
        }
        else
        {
            u8MBStatus = 0;
            newpacket = true;
        }
    }
  }
  // end while not done
  

  // OK, we have a valid packet. If it has data, load it into the buffer 
  if(!u8MBStatus)
  {
    // evaluate returned Modbus function code
    // it's going to be different for client and server
    if(isClient)
    {
        if(LastReceivedFunctionCode== MODBUS04_ReadInputRegisters || LastReceivedFunctionCode == MODBUS03_ReadHoldingRegisters)
        {
        
      
           // load bytes into word; response bytes are ordered H, L, H, L, ...
            for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
            {
            if (i < ku8MaxBufferSize)
            {
                _u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 3], u8ModbusADU[2 * i + 4]);
            }
          
            _u8ResponseBufferLength = i;
        }

    }
    }
    else // is Server
    {
        if (LastReceivedFunctionCode == MODBUS04_ReadInputRegisters || LastReceivedFunctionCode == MODBUS03_ReadHoldingRegisters)
        {
            _u16ResponseBuffer[0] = word(u8ModbusADU[2], u8ModbusADU[3]);
            _u16ResponseBuffer[1] = word(u8ModbusADU[4], u8ModbusADU[5]);
            _u8ResponseBufferLength = 2;
        }
        if (LastReceivedFunctionCode == MODBUS10_WriteMultipleRegisters)
        {

            //byte 5 is the number of words of data
            // data starts at byte 7
            // load bytes into word; response bytes are ordered H, L, H, L, ...
            // need address and count
            _u16ResponseBuffer[0] = word(u8ModbusADU[2], u8ModbusADU[3]);
            _u16ResponseBuffer[1] = word(u8ModbusADU[4], u8ModbusADU[5]);

            for (i = 0; i < u8ModbusADU[5]; i++)
            {
                if (i+1 < ku8MaxBufferSize)
                {
                    _u16ResponseBuffer[i+2] = word(u8ModbusADU[2 * i + 7], u8ModbusADU[2 * i + 8]);
                }

                _u8ResponseBufferLength = 2;
            }
        }
    }
  }
  
  _u8TransmitBufferIndex = 0;
  u16TransmitBufferLength = 0;
  _u8ResponseBufferIndex = 0;

  if (u8MBStatus == ku8MBSuccess)
  {


      
      

      // check whether Modbus exception occurred; return Modbus Exception Code
      if (bitRead(LastReceivedFunctionCode, 7))
      {
          u8MBStatus = u8ModbusADU[2];
          Serial.println("Modbus exception");
      }
      
  }
  return u8MBStatus;
}

void ModbusFrame::SetPins(uint32_t DE, uint32_t RE)
{
    m_DE = DE;
    m_RE = RE;
    pinMode(DE, OUTPUT);
    pinMode(RE, OUTPUT);
    
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    
}

/**
Place data in transmit buffer.

@see ModbusMaster::clearTransmitBuffer()
@param u8Index index of transmit buffer array (0x00..0x3F)
@param u16Value value to place in position u8Index of transmit buffer (0x0000..0xFFFF)
@return 0 on success; exception number on failure
@ingroup buffer
*/
uint8_t ModbusFrame::TransmitBufferPutAt(uint8_t u8Index, uint16_t u16Value)
{
    if (u8Index < ku8MaxBufferSize)
    {
        _u16TransmitBuffer[u8Index] = u16Value;
        return ku8MBSuccess;
    }
    else
    {
        return ku8MBIllegalDataAddress;
    }
}

uint16_t ModbusFrame::ResponseBufferGetAt(uint8_t u8Index)
{
    if (u8Index < ku8MaxBufferSize)
    {
        return _u16ResponseBuffer[u8Index];
    }
    else
    {
        return 0xFFFF;
    }
}

uint8_t ModbusFrame::ReceivedAddress()
{
    return LastReceivedAddress;
    

    
};
uint8_t ModbusFrame::ReceivedFunctionCode()
{
    return LastReceivedFunctionCode;
};
