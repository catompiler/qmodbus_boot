#ifndef MODBUSERR_H
#define MODBUSERR_H

#include <QSharedData>
#include <QSharedDataPointer>
#include <QString>
#include <QModbusDevice>
#include <QModbusPdu>

class ModbusDev;

class ModbusErr
{
public:

    enum Type{
        None = 0,
        General, // Общая ошибка.
        State, // Ошибка состояния.
        Modbus // Ошибка Modbus.
    };

    ModbusErr();
    ModbusErr(Type t, const QString& sndr, const QString& estr);
    ModbusErr(const ModbusErr& me);
    ModbusErr(const ModbusErr&& me);
    ~ModbusErr();

    ModbusDev* modbusDev();
    void setModbusDev(ModbusDev* dev);

    const QString& sanderName() const;
    void setSanderName(const QString& sndr);

    Type type() const;
    void setType(Type t);

    const QString& errorStr() const;
    void setErrorStr(const QString& estr);

    QModbusPdu::ExceptionCode modbusException() const;
    void setModbusException(QModbusPdu::ExceptionCode mexc);

    QModbusDevice::Error modbusError() const;
    void setModbusError(QModbusDevice::Error merr);

    const QString& modbusErrorStr() const;
    void setModbusErrorStr(const QString& mestr);

private:

    class ModbusErrData
        :public QSharedData
    {
    public:
        ModbusErrData();
        ModbusErrData(Type t, const QString& sndr, const QString& estr);
        ModbusErrData(const ModbusErrData& med);
        ~ModbusErrData();

        ModbusDev* modbus_dev;
        QString sander_name;

        Type err_type;
        QString err_str;

        QModbusPdu::ExceptionCode modbus_exc;
        QModbusDevice::Error modbus_err;
        QString modbus_err_str;
    };

    QSharedDataPointer<ModbusErrData> data;
};

Q_DECLARE_METATYPE(ModbusErr)

#endif // MODBUSERR_H
