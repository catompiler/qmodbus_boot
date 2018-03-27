#include "modbusreg.h"
#include "modbusmsg.h"
#include <QModbusReply>
#include <QModbusClient>
#include <QDebug>


ModbusReg::ModbusReg(QObject *parent) : ModbusObj(parent)
{
    reg_type = QModbusDataUnit::Invalid;
    reg_address = 0;
}

ModbusReg::ModbusReg(ModbusDev* dev, QModbusDataUnit::RegisterType type, int reg_addr, int count, QObject *parent) : ModbusObj(dev, parent)
{
    reg_type = type;
    reg_address = reg_addr;
    reg_data.resize(count);
}

ModbusReg::~ModbusReg()
{
}

QModbusDataUnit::RegisterType ModbusReg::regType() const
{
    return reg_type;
}

void ModbusReg::setRegType(QModbusDataUnit::RegisterType type)
{
    reg_type = type;
}

int ModbusReg::regAddress() const
{
    return reg_address;
}

void ModbusReg::setRegAddress(int reg_addr)
{
    reg_address = reg_addr;
}

int ModbusReg::regCount() const
{
    return reg_data.size();
}

void ModbusReg::setRegCount(int count)
{
    reg_data.resize(count);
}

const uint16_t& ModbusReg::data(int i) const
{
    return reg_data[i];
}

uint16_t &ModbusReg::data(int i)
{
    return reg_data[i];
}

void ModbusReg::setData(int i, uint16_t val)
{
    reg_data[i] = val;
}

uint16_t ModbusReg::value() const
{
    if(reg_data.empty()) return 0;
    return reg_data[0];
}

void ModbusReg::setValue(uint16_t val)
{
    if(!reg_data.empty()){
        reg_data[0] = val;
    }
}

bool ModbusReg::read()
{
    if(!modbusDev() || !modbusDev()->isValid()) return false;

    ModbusMsg* modbus_msg = new ModbusMsg();

    QModbusDataUnit du(reg_type, reg_address, reg_data.size());

    modbus_msg->setDataUnit(du, ModbusMsg::Read);

    connect(modbus_msg, &ModbusMsg::sendSuccess, this, &ModbusReg::msgDataReaded);
    connect(modbus_msg, &ModbusMsg::sendError, this, &ModbusReg::msgError);

    if(!modbusDev()->sendMsg(modbus_msg)){
        delete modbus_msg;
        return false;
    }

    return true;
}

bool ModbusReg::write()
{
    if(!modbusDev() || !modbusDev()->isValid()) return false;

    ModbusMsg* modbus_msg = new ModbusMsg();

    QModbusDataUnit du(reg_type, reg_address, reg_data);

    modbus_msg->setDataUnit(du, ModbusMsg::Write);

    connect(modbus_msg, &ModbusMsg::sendSuccess, this, &ModbusReg::msgDataWrited);
    connect(modbus_msg, &ModbusMsg::sendError, this, &ModbusReg::msgError);

    if(!modbusDev()->sendMsg(modbus_msg)){
        delete modbus_msg;
        return false;
    }

    return true;
}

void ModbusReg::msgError(ModbusErr error)
{
    ModbusMsg* msg = qobject_cast<ModbusMsg*>(sender());
    if(!msg){
        qDebug() << "ModbusReg: msgError msg == NULL!";
        return;
    }

    msg->deleteLater();

    emit errorOccured(error);
}

void ModbusReg::msgDataReaded()
{
    ModbusMsg* msg = qobject_cast<ModbusMsg*>(sender());
    if(!msg){
        qDebug() << "ModbusReg: msgDataReaded msg == NULL!";
        return;
    }

    msg->deleteLater();

    if(!msg->isSended()) return;

    QModbusReply* reply = msg->reply();
    if(!reply){
        qDebug() << "ModbusReg: msgDataReaded reply == NULL!";
        emit errorOccured(ModbusErr(ModbusErr::State, tr("ModbusReg"), tr("Read reply == nullptr!")));
        return;
    }

    if(reply->error() != QModbusDevice::NoError){
        emit errorOccured(ModbusErr(ModbusErr::State, tr("ModbusReg"), tr("Read reply has error!")));
        return;
    }

    QModbusDataUnit du = reply->result();
    if(!du.isValid()){
        emit errorOccured(ModbusErr(ModbusErr::State, tr("ModbusReg"), tr("Read result invalid!")));
        return;
    }

    QVector<uint16_t> values = du.values();
    if(values.size() != reg_data.size()){
        emit errorOccured(ModbusErr(ModbusErr::State, tr("ModbusReg"), tr("Read size mismatch!")));
        return;
    }

    reg_data = values;

    emit dataReaded();
}

void ModbusReg::msgDataWrited()
{
    ModbusMsg* msg = qobject_cast<ModbusMsg*>(sender());
    if(!msg){
        qDebug() << "ModbusReg: msgDataWrited msg == NULL!";
        return;
    }

    msg->deleteLater();

    if(!msg->isSended()) return;

    QModbusReply* reply = msg->reply();
    if(!reply){
        qDebug() << "ModbusReg: msgDataWrited reply == NULL!";
        emit errorOccured(ModbusErr(ModbusErr::State, tr("ModbusReg"), tr("Write reply == nullptr!")));
        return;
    }

    if(reply->error() != QModbusDevice::NoError){
        emit errorOccured(ModbusErr(ModbusErr::State, tr("ModbusReg"), tr("Write reply has error!")));
        return;
    }

    emit dataWrited();
}

