#include "modbusnet.h"
#include <QModbusRtuSerialMaster>
#include "settings.h"
#include "modbusmsg.h"
#include <QVariant>
#include <QDebug>
#include <QDebug>


#define MAX_PDU_SIZE 253


ModbusNet::ModbusNet(QObject *parent) : QObject(parent)
{
    msg_queue = new MsgQueue();
    modbus = nullptr;
}

ModbusNet::~ModbusNet()
{
    disconnectFromNet();
    if(modbus) delete modbus;
    delete msg_queue;
}

bool ModbusNet::setup()
{
    Settings& settings = Settings::get();

    QModbusRtuSerialMaster* modbus_rtu = new QModbusRtuSerialMaster(this);

    connect(modbus_rtu, &QModbusDevice::stateChanged, this, &ModbusNet::on_modbus_state_changed);
    connect(modbus_rtu, &QModbusDevice::errorOccurred, this, &ModbusNet::on_modbus_error_occured);

    modbus_rtu->setConnectionParameter(QModbusDevice::SerialPortNameParameter, settings.serialPortName());
    modbus_rtu->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, settings.serailPortBaud());
    modbus_rtu->setConnectionParameter(QModbusDevice::SerialParityParameter, settings.serialPortParity());
    modbus_rtu->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, settings.serialPortStopBits());
    modbus_rtu->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8); // 7 or 9 bit? O'Rly?

    modbus_rtu->setInterFrameDelay(settings.modbusFrameDelay());
    modbus_rtu->setTimeout(settings.modbusTimeout());
    modbus_rtu->setNumberOfRetries(settings.modbusRetries());

    if(modbus) delete modbus;
    modbus = modbus_rtu;

    return true;
}

bool ModbusNet::connectToNet()
{
    if(!modbus) return false;

    if(!modbus->connectDevice()){
        return false;
    }

    return true;
}

void ModbusNet::disconnectFromNet()
{
    if(!modbus) return;

    modbus->disconnectDevice();

    clearQueue();
}

bool ModbusNet::isConnectedToNet()
{
    if(!modbus) return false;

    return modbus->state() == QModbusDevice::ConnectedState;
}

int ModbusNet::maxPduSize() const
{
    return MAX_PDU_SIZE;
}

bool ModbusNet::sendMsg(ModbusMsg *msg, int slaveAddr)
{
    if(!modbus) return false;
    if(!isConnectedToNet()) return false;

    bool need_send = msg_queue->empty();

    msg_queue->append(MsgPair(msg, slaveAddr));

    if(need_send){
        sendNextMsg();
    }

    return true;
}

void ModbusNet::on_modbus_state_changed(QModbusDevice::State state)
{
    switch(state){
    default:
        break;
    case QModbusDevice::ConnectedState:
        emit connectedToNet();
        break;
    case QModbusDevice::UnconnectedState:
        emit disconnectedFromNet();
        break;
    }

    emit stateChanged(state);
}

void ModbusNet::on_modbus_error_occured(QModbusDevice::Error error)
{
    ModbusErr err(ModbusErr::Modbus, tr("ModbusNet"), tr("Modbus error!"));

    err.setModbusError(error);

    if(modbus) err.setModbusErrorStr(modbus->errorString());

    emit errorOccured(err);
}

void ModbusNet::on_queue_msg_finished()
{
    if(msg_queue->empty()){
        qDebug() << "ModbusNet: on_queue_msg_sended with empty queue!";
        return;
    }

    MsgPair& msg_pair = msg_queue->first();
    ModbusMsg* msg = msg_pair.first;

    disconnect(msg, &ModbusMsg::finished, this, &ModbusNet::on_queue_msg_finished);

    msg_queue->removeFirst();

    sendNextMsg();
}

bool ModbusNet::sendNextMsg()
{
    if(!modbus) return false;
    if(!isConnectedToNet()) return false;

    for(;;){
        if(msg_queue->empty()) return false;

        MsgPair& msg_pair = msg_queue->first();

        ModbusMsg* msg = msg_pair.first;
        int slaveAddr = msg_pair.second;

        connect(msg, &ModbusMsg::finished, this, &ModbusNet::on_queue_msg_finished);

        if(msg->send(modbus, slaveAddr)) break;

        disconnect(msg, &ModbusMsg::finished, this, &ModbusNet::on_queue_msg_finished);

        msg_queue->removeFirst();
    }

    return true;
}

void ModbusNet::clearQueue()
{
    if(msg_queue->empty()) return;

    disconnect(msg_queue->takeFirst().first, &ModbusMsg::finished, this, &ModbusNet::on_queue_msg_finished);

    for(MsgPair& msg_pair: *msg_queue){
        msg_pair.first->cancel();
    }

    msg_queue->clear();
}
