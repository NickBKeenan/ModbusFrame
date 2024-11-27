#include "ModbusServer.h"

void ModbusServer::begin(uint8_t p_ServerID, Stream& serial)
{

	 ServerID = p_ServerID;

	ModbusFrame::begin(serial);

};

void ModbusServer::ServiceAnyRequests()
{

	while(true)
	{ 
	if (!_serial->available())
	{
		return;

	}

	Serial.println("Servicing");
	if (ReceiveFrame(ServerID, false)==0)
	{
			RequestReaction();
			Serial.print(".");
		}
	}
}