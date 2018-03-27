#ifndef MODBUSDEV_H
#define MODBUSDEV_H

#include <QObject>
#include "modbusnet.h"

class ModbusMsg;

class ModbusDev : public QObject
{
    Q_OBJECT
public:
    explicit ModbusDev(QObject *parent = 0);
    ModbusDev(ModbusNet* net, int slave_addr, QObject *parent = 0);
    virtual ~ModbusDev();

    bool isValid() const;

    ModbusNet *modbusNet();
    void setModbusNet(ModbusNet* net);

    int slaveAddress() const;
    void setSlaveAddress(int slave_addr);

    int maxPduSize() const;

    bool sendMsg(ModbusMsg* msg);

signals:

public slots:

protected:
    ModbusNet* modbus_net;
    int slave_address;
};

#endif // MODBUSDEV_H
