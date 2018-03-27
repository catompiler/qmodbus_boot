#ifndef MODBUSOBJ_H
#define MODBUSOBJ_H

#include <QObject>
#include "modbusdev.h"

class ModbusObj : public QObject
{
    Q_OBJECT
public:
    explicit ModbusObj(QObject *parent = 0);
    ModbusObj(ModbusDev* dev, QObject *parent = 0);
    virtual ~ModbusObj();

    ModbusDev *modbusDev();
    void setModbusDev(ModbusDev* dev);

signals:

public slots:

protected:
    ModbusDev* modbus_dev;
};

#endif // MODBUSOBJ_H
