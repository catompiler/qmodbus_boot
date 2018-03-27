#include "modbusmsg.h"
#include <QModbusClient>
#include <QModbusReply>
#include <QDebug>


ModbusMsg::ModbusMsg(QObject *parent) : QObject(parent)
{
    msg_state = Idle;
    msg_sender = nullptr;
    modbus_reply = nullptr;
}

ModbusMsg::ModbusMsg(const QModbusRequest& req, QObject *parent)
    :QObject(parent)
{
    msg_state = Idle;
    msg_sender = nullptr;
    modbus_reply = nullptr;
    setRequest(req);
}

ModbusMsg::ModbusMsg(const QModbusDataUnit& du, DataUnitDirection d, QObject *parent)
    :QObject(parent)
{
    msg_state = Idle;
    msg_sender = nullptr;
    modbus_reply = nullptr;
    setDataUnit(du, d);
}

ModbusMsg::~ModbusMsg()
{
    cleanup();
}

bool ModbusMsg::setRequest(const QModbusRequest &req)
{
    if(isSending()) return false;

    cleanup();

    msg_sender = new MsgRawRequest(req);

    return true;
}

bool ModbusMsg::setDataUnit(const QModbusDataUnit &du, ModbusMsg::DataUnitDirection d)
{
    if(isSending()) return false;

    cleanup();

    msg_sender = new MsgDataUnit(du, d);

    return true;
}

bool ModbusMsg::isValid() const
{
    return msg_sender != nullptr;
}

ModbusMsg::State ModbusMsg::state() const
{
    return msg_state;
}

bool ModbusMsg::isSended() const
{
    return msg_state == Sended;
}

bool ModbusMsg::isSending() const
{
    return msg_state == Sending;
}

bool ModbusMsg::isCanceled() const
{
    return msg_state == Canceled;
}

QModbusReply *ModbusMsg::reply()
{
    return modbus_reply;
}

bool ModbusMsg::clear()
{
    if(isSending()) return false;

    cleanup();

    msg_state = Idle;

    return true;
}

bool ModbusMsg::send(QModbusClient *modbus, int slaveAddr)
{
    if(isSending()){
        qDebug() << "ModbusMsg: send sending message!";
        onSendFail(ModbusErr(ModbusErr::State, tr("ModbusMsg"), tr("Send sending message!")));
        return false;
    }

    if(!isValid()){
        qDebug() << "ModbusMsg: send invalid message!";
        onSendFail(ModbusErr(ModbusErr::State, tr("ModbusMsg"), tr("Send invalid message!")));
        return false;
    }

    msg_state = Sending;

    cleanupReply();

    modbus_reply = msg_sender->send(modbus, slaveAddr);

    if(!modbus_reply){
        onSendFail(ModbusErr(ModbusErr::State, tr("ModbusMsg"), tr("QModbusClient returns nullptr reply!")));
        return false;
    }

    if(!modbus_reply->isFinished()){
        connect(modbus_reply, &QModbusReply::finished, this, &ModbusMsg::reqFinished);
        connect(modbus_reply, &QModbusReply::errorOccurred, this, &ModbusMsg::reqError);
    }else{
        onSendDone();
    }

    return true;
}

bool ModbusMsg::cancel()
{
    if(isSending()){
        qDebug() << "ModbusMsg: cancel sending message!";
        return false;
    }
    onSendCanceled();

    return true;
}

void ModbusMsg::reqFinished()
{
    onSendDone();
}

void ModbusMsg::reqError(QModbusDevice::Error err_code)
{
    ModbusErr err(ModbusErr::Modbus, tr("ModbusMsg"), tr("Modbus error!"));

    err.setModbusError(err_code);

    if(modbus_reply){

        err.setModbusErrorStr(modbus_reply->errorString());

        QModbusResponse resp = modbus_reply->rawResult();

        //if(resp.isValid() && resp.isException()){
            err.setModbusException(resp.exceptionCode());
        //}
    }

    onSendError(err);
}

void ModbusMsg::cleanup()
{
    cleanupReply();
    cleanupSender();
}

void ModbusMsg::cleanupSender()
{
    if(msg_sender){
        delete msg_sender;
        msg_sender = nullptr;
    }
}

void ModbusMsg::cleanupReply()
{
    if(modbus_reply){
        modbus_reply->deleteLater();
        modbus_reply = nullptr;
    }
}

void ModbusMsg::onSendFail(const ModbusErr& err)
{
    onSendError(err);
    onSendDone();
}

void ModbusMsg::onSendCanceled()
{
    msg_state = Canceled;
    emit sendCancel();

    onSendDone();
}

void ModbusMsg::onSendDone()
{
    switch (msg_state) {
    case Sending:
        msg_state = Sended;
        emit sendSuccess();
        break;
    case Error:
    case Canceled:
        break;
    default:
        qDebug() << "ModbusMsg: onSendFinished invalid message state: " << msg_state;
        break;
    }

    emit finished();
}

void ModbusMsg::onSendError(const ModbusErr& err)
{
    msg_state = Error;
    emit sendError(err);
}


ModbusMsg::MsgRawRequest::MsgRawRequest(const QModbusRequest &req) : MsgSender()
{
    modbus_req = req;
}

ModbusMsg::MsgRawRequest::~MsgRawRequest()
{
}

QModbusReply *ModbusMsg::MsgRawRequest::send(QModbusClient *modbus, int modbus_slave)
{
    if(!modbus_req.isValid()) return nullptr;

    return modbus->sendRawRequest(modbus_req, modbus_slave);
}

ModbusMsg::MsgDataUnit::MsgDataUnit(const QModbusDataUnit &du, ModbusMsg::DataUnitDirection d) : MsgSender()
{
    modbus_du = du;
    dir = d;
}

ModbusMsg::MsgDataUnit::~MsgDataUnit()
{
}

QModbusReply *ModbusMsg::MsgDataUnit::send(QModbusClient *modbus, int modbus_slave)
{
    switch(dir){
    default:
        break;
    case ModbusMsg::Read:
        return modbus->sendReadRequest(modbus_du, modbus_slave);
    case ModbusMsg::Write:
        return modbus->sendWriteRequest(modbus_du, modbus_slave);
    }
    return nullptr;
}
