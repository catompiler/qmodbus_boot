#ifndef MODBUSNET_H
#define MODBUSNET_H

#include <QObject>
#include <QModbusDevice>
#include <QString>
#include <QQueue>
#include <QPair>
#include "modbuserr.h"

class QModbusClient;
class ModbusMsg;


class ModbusNet : public QObject
{
    Q_OBJECT
public:
    explicit ModbusNet(QObject *parent = 0);
    ~ModbusNet();

    bool setup();

    bool connectToNet();
    void disconnectFromNet();

    bool isConnectedToNet();

    int maxPduSize() const;

    /*
     * Возвращает истину после добавления сообщения в очередь,
     * не зависимо от результатов передачи.
     * Для контроля отправки сообщения в сеть необходимо
     * использовать сигналы сообщения.
     */
    bool sendMsg(ModbusMsg* msg, int slaveAddr);

signals:
    void stateChanged(QModbusDevice::State state);
    void errorOccured(ModbusErr error);

    // Вызываются после сигнала stateChanged.
    void connectedToNet();
    void disconnectedFromNet();

public slots:

private slots:
    void on_modbus_state_changed(QModbusDevice::State state);
    void on_modbus_error_occured(QModbusDevice::Error error);

    void on_queue_msg_finished();

private:
    QModbusClient* modbus;

    typedef QPair<ModbusMsg*, int> MsgPair;
    typedef QQueue<MsgPair> MsgQueue;
    MsgQueue* msg_queue;

    bool sendNextMsg();
    void clearQueue();
};

#endif // MODBUSNET_H
