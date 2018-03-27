#ifndef MODBUSREG_H
#define MODBUSREG_H

#include "modbusobj.h"
#include "modbuserr.h"
#include <QModbusDataUnit>
#include <QVector>

class ModbusMsg;

class ModbusReg : public ModbusObj
{
    Q_OBJECT
public:

    explicit ModbusReg(QObject *parent = 0);
    ModbusReg(ModbusDev* dev, QModbusDataUnit::RegisterType type, int reg_addr, int count = 1, QObject *parent = 0);
    ~ModbusReg();

    QModbusDataUnit::RegisterType regType() const;
    void setRegType(QModbusDataUnit::RegisterType type);

    int regAddress() const;
    void setRegAddress(int reg_addr);

    int regCount() const;
    void setRegCount(int count);

    const uint16_t &data(int i) const;
    uint16_t &data(int i);
    void setData(int i, uint16_t val);

    uint16_t value() const;
    void setValue(uint16_t val);

    bool read();
    bool write();

signals:
    void errorOccured(ModbusErr error);
    void dataReaded();
    void dataWrited();

private slots:
    void msgError(ModbusErr error);
    void msgDataReaded();
    void msgDataWrited();

private:
    QModbusDataUnit::RegisterType reg_type;
    int reg_address;
    QVector<uint16_t> reg_data;
};

#endif // MODBUSREG_H
