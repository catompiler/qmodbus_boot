#ifndef MODBUSMSG_H
#define MODBUSMSG_H

#include <QObject>
#include <QModbusDevice>
#include <QModbusRequest>
#include <QModbusDataUnit>
#include "modbuserr.h"

class QModbusClient;
class QModbusReply;

class ModbusMsg : public QObject
{
    Q_OBJECT
public:

    enum State {
        Idle,
        Sending,
        Sended,
        Error,
        Canceled
    };
    Q_ENUM(State)

    enum DataUnitDirection {
        Read,
        Write
    };
    Q_ENUM(DataUnitDirection)

    explicit ModbusMsg(QObject *parent = 0);
    ModbusMsg(const QModbusRequest& req, QObject *parent = 0);
    ModbusMsg(const QModbusDataUnit& du, DataUnitDirection d, QObject *parent = 0);
    ~ModbusMsg();

    bool setRequest(const QModbusRequest& req);
    bool setDataUnit(const QModbusDataUnit& du, DataUnitDirection d);

    bool isValid() const;

    State state() const;

    bool isSended() const;
    bool isSending() const;
    bool isCanceled() const;

    QModbusReply* reply();

    bool clear();

    bool send(QModbusClient* modbus, int slaveAddr);

    int dataSize() const;

    bool cancel();

signals:
    void finished();
    void sendSuccess();
    void sendError(ModbusErr err);
    void sendCancel();

private slots:
    void reqFinished();
    void reqError(QModbusDevice::Error err_code);

private:

    class MsgSender {
    public:
        MsgSender(){}
        virtual ~MsgSender(){}

        virtual QModbusReply* send(QModbusClient *modbus, int modbus_slave) = 0;
        virtual int dataSize() const = 0;
    };

    class MsgRawRequest : public MsgSender {
    public:
        MsgRawRequest(const QModbusRequest& req);
        ~MsgRawRequest();

        QModbusReply* send(QModbusClient *modbus, int modbus_slave);
        virtual int dataSize() const;

    private:
        QModbusRequest modbus_req;
    };

    class MsgDataUnit : public MsgSender {
    public:

        MsgDataUnit(const QModbusDataUnit& du, DataUnitDirection d);
        ~MsgDataUnit();

        QModbusReply* send(QModbusClient *modbus, int modbus_slave);
        virtual int dataSize() const;

    private:
        QModbusDataUnit modbus_du;
        DataUnitDirection dir;
    };

    void cleanup();
    void cleanupSender();
    void cleanupReply();

    void onSendFail(const ModbusErr& err);
    void onSendCanceled();
    void onSendDone();
    void onSendError(const ModbusErr& err);

    State msg_state;
    MsgSender* msg_sender;
    QModbusReply* modbus_reply;
};

#endif // MODBUSMSG_H
