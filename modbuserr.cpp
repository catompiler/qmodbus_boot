#include "modbuserr.h"
#include <algorithm>



ModbusErr::ModbusErr()
{
    data = new ModbusErrData();
}

ModbusErr::ModbusErr(ModbusErr::Type t, const QString& sndr, const QString& estr)
{
    data = new ModbusErrData(t, sndr, estr);
}

ModbusErr::ModbusErr(const ModbusErr& me)
    :data(me.data)
{
}

ModbusErr::ModbusErr(const ModbusErr&& me)
    :data(std::move(me.data))
{
}

ModbusErr::~ModbusErr()
{
}

ModbusDev* ModbusErr::modbusDev()
{
    return data->modbus_dev;
}

void ModbusErr::setModbusDev(ModbusDev* dev)
{
    data->modbus_dev = dev;
}

const QString& ModbusErr::sanderName() const
{
    return data->sander_name;
}

void ModbusErr::setSanderName(const QString& sndr)
{
    data->sander_name = sndr;
}

ModbusErr::Type ModbusErr::type() const
{
    return data->err_type;
}

void ModbusErr::setType(ModbusErr::Type t)
{
    data->err_type = t;
}

const QString& ModbusErr::errorStr() const
{
    return data->err_str;
}

void ModbusErr::setErrorStr(const QString& estr)
{
    data->err_str = estr;
}

QModbusPdu::ExceptionCode ModbusErr::modbusException() const
{
    return data->modbus_exc;
}

void ModbusErr::setModbusException(QModbusPdu::ExceptionCode mexc)
{
    data->modbus_exc = mexc;
}

QModbusDevice::Error ModbusErr::modbusError() const
{
    return data->modbus_err;
}

void ModbusErr::setModbusError(QModbusDevice::Error merr)
{
    data->modbus_err = merr;
}

const QString& ModbusErr::modbusErrorStr() const
{
    return data->modbus_err_str;
}

void ModbusErr::setModbusErrorStr(const QString& mestr)
{
    data->modbus_err_str = mestr;
}

ModbusErr::ModbusErrData::ModbusErrData()
    :QSharedData()
{
    modbus_dev = nullptr;
    err_type = None;
    sander_name = QString();
    err_str = QString();
    modbus_exc = static_cast<QModbusPdu::ExceptionCode>(0);
    modbus_err = QModbusDevice::NoError;
    modbus_err_str = QString();
}

ModbusErr::ModbusErrData::ModbusErrData(ModbusErr::Type t, const QString& sndr, const QString& estr)
    :QSharedData()
{
    modbus_dev = nullptr;
    err_type = t;
    sander_name = sndr;
    err_str = estr;
    modbus_exc = static_cast<QModbusPdu::ExceptionCode>(0);
    modbus_err = QModbusDevice::NoError;
    modbus_err_str = QString();
}

ModbusErr::ModbusErrData::ModbusErrData(const ModbusErr::ModbusErrData& med)
    :QSharedData(med)
{
    modbus_dev = med.modbus_dev;
    sander_name = med.sander_name;
    err_type = med.err_type;
    err_str = med.err_str;
    modbus_exc = med.modbus_exc;
    modbus_err = med.modbus_err;
    modbus_err_str = med.modbus_err_str;
}

ModbusErr::ModbusErrData::~ModbusErrData()
{
}
