#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSerialPort>


class Settings : public QObject
{
    Q_OBJECT
public:

    static Settings& get();
    ~Settings();

    void read();
    void write();

    // Порт.
    const QString& serialPortName()            const { return m_serial_name; }
    quint32 serailPortBaud()                   const { return m_serial_baud; }
    QSerialPort::Parity serialPortParity()     const { return m_serial_parity; }
    QSerialPort::StopBits serialPortStopBits() const { return m_serial_stopbits; }

    // Протокол.
    quint32 modbusSlaveAddress() const { return m_modbus_slave; }
    quint32 modbusTimeout()      const { return m_modbus_timeout; }
    quint32 modbusFrameDelay()   const { return m_modbus_frame_delay; }
    quint32 modbusRetries()      const { return m_modbus_retries; }

public slots:
    // Порт.
    void setSerialPortName(const QString& val);
    void setSerialPortBaud(quint32 val);
    void setSerialPortParity(QSerialPort::Parity val);
    void setSerialPortStopBits(QSerialPort::StopBits val);

    // Протокол.
    void setModbusSlaveAddress(quint32 val);
    void setModbusTimeout(quint32 val);
    void setModbusFrameDelay(quint32 val);
    void setModbusRetries(quint32 val);

signals:

private:
    explicit Settings(QObject *parent = 0);

    // Порт.
    QString m_serial_name;
    quint32 m_serial_baud;
    QSerialPort::Parity m_serial_parity;
    QSerialPort::StopBits m_serial_stopbits;
    // Протокол.
    quint32 m_modbus_slave;
    quint32 m_modbus_timeout;
    quint32 m_modbus_frame_delay;
    quint32 m_modbus_retries;
};

#endif // SETTINGS_H
