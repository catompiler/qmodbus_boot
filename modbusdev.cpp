#include "modbusdev.h"
#include "modbusmsg.h"

ModbusDev::ModbusDev(QObject *parent) : QObject(parent)
{
    modbus_net = nullptr;
    slave_address = 0;
}

ModbusDev::ModbusDev(ModbusNet* net, int slave_addr, QObject *parent) : QObject(parent)
{
    modbus_net = net;
    slave_address = slave_addr;
}

ModbusDev::~ModbusDev()
{
}

bool ModbusDev::isValid() const
{
    return modbus_net != nullptr;
}

ModbusNet *ModbusDev::modbusNet()
{
    return modbus_net;
}

void ModbusDev::setModbusNet(ModbusNet *net)
{
    modbus_net = net;
}

int ModbusDev::slaveAddress() const
{
    return slave_address;
}

void ModbusDev::setSlaveAddress(int slave_addr)
{
    slave_address = slave_addr;
}

int ModbusDev::maxPduSize() const
{
    if(!modbus_net) return 0;

    return modbus_net->maxPduSize();
}

bool ModbusDev::sendMsg(ModbusMsg* msg)
{
    if(!modbus_net) return false;
    if(!msg->isValid()) return false;
    if(msg->isSending()) return false;

    return modbus_net->sendMsg(msg, slave_address);
}
