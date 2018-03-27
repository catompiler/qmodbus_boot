#include "settings.h"
#include <QSettings>
#include <QSerialPortInfo>


#define S(str) QStringLiteral(str)

#define SERIAL_NAME S("serial_name")
#define SERIAL_BAUD S("serial_baud")
#define SERIAL_PARITY S("serial_parity")
#define SERIAL_STOPBITS S("serial_stopbits")

#define MODBUS_SLAVE S("modbus_slave")
#define MODBUS_TIMEOUT S("modbus_timeout")
#define MODBUS_FRAME_DELAY S("modbus_frame_delay")
#define MODBUS_RETRIES S("modbus_retries")


Settings::Settings(QObject *parent) : QObject(parent)
{
}

Settings &Settings::get()
{
    static Settings settings;

    return settings;
}

Settings::~Settings()
{
}

void Settings::read()
{
    QSettings settings;

    QString defSerialName;

    auto portsInfo = QSerialPortInfo::availablePorts();
    if(!portsInfo.empty()) defSerialName = portsInfo.first().portName();

    m_serial_name = settings.value(SERIAL_NAME, defSerialName).toString();
    m_serial_baud = settings.value(SERIAL_BAUD, 9600).toUInt();
    m_serial_parity = static_cast<QSerialPort::Parity>(settings.value(SERIAL_PARITY, 0).toUInt());
    m_serial_stopbits = static_cast<QSerialPort::StopBits>(settings.value(SERIAL_STOPBITS, 0).toUInt());

    m_modbus_slave = settings.value(MODBUS_SLAVE, 1).toUInt();
    m_modbus_timeout = settings.value(MODBUS_TIMEOUT, 500).toUInt();
    m_modbus_frame_delay = settings.value(MODBUS_FRAME_DELAY, 10000000).toUInt();
    m_modbus_retries = settings.value(MODBUS_RETRIES, 10).toUInt();
}

void Settings::write()
{
    QSettings settings;

    settings.clear();

    settings.setValue(SERIAL_NAME, m_serial_name);
    settings.setValue(SERIAL_BAUD, m_serial_baud);
    settings.setValue(SERIAL_PARITY, static_cast<quint32>(m_serial_parity));
    settings.setValue(SERIAL_STOPBITS, static_cast<quint32>(m_serial_stopbits));

    settings.setValue(MODBUS_SLAVE, m_modbus_slave);
    settings.setValue(MODBUS_TIMEOUT, m_modbus_timeout);
    settings.setValue(MODBUS_FRAME_DELAY, m_modbus_frame_delay);
    settings.setValue(MODBUS_RETRIES, m_modbus_retries);
}

void Settings::setSerialPortName(const QString &val)
{
    m_serial_name = val;
}

void Settings::setSerialPortBaud(quint32 val)
{
    m_serial_baud = val;
}

void Settings::setSerialPortParity(QSerialPort::Parity val)
{
    m_serial_parity = val;
}

void Settings::setSerialPortStopBits(QSerialPort::StopBits val)
{
    m_serial_stopbits = val;
}

void Settings::setModbusSlaveAddress(quint32 val)
{
    m_modbus_slave = val;
}

void Settings::setModbusTimeout(quint32 val)
{
    m_modbus_timeout = val;
}

void Settings::setModbusFrameDelay(quint32 val)
{
    m_modbus_frame_delay = val;
}

void Settings::setModbusRetries(quint32 val)
{
    m_modbus_retries = val;
}
