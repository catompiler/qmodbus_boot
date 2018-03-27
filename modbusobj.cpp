#include "modbusobj.h"

ModbusObj::ModbusObj(QObject *parent) : QObject(parent)
{
    modbus_dev = nullptr;
}

ModbusObj::ModbusObj(ModbusDev* dev, QObject *parent) : QObject(parent)
{
    modbus_dev = dev;
}

ModbusObj::~ModbusObj()
{
}

ModbusDev *ModbusObj::modbusDev()
{
    return modbus_dev;
}

void ModbusObj::setModbusDev(ModbusDev *dev)
{
    modbus_dev = dev;
}

