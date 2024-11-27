#pragma once
 
#include "ModbusFrame.h"

class ModbusServer: public ModbusFrame
{
    uint8_t ServerID;
  public:
    
   
    void begin(uint8_t ServerID, Stream &serial);
    void ServiceAnyRequests();
    virtual void RequestReaction()=0;
    
    

    
    
  
    
    
};