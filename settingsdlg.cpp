#include "settingsdlg.h"
#include "ui_settingsdlg.h"
#include <QSerialPortInfo>
#include <QComboBox>
#include <QString>
#include <QCompleter>
#include "settings.h"


SettingsDlg::SettingsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);

    populateDevicesList();
    populateBaudsList();
}

SettingsDlg::~SettingsDlg()
{
    delete ui;
}

void SettingsDlg::loadSettings()
{
    Settings& settings = Settings::get();

    setCurrentItemByText(ui->cbDevice, settings.serialPortName());
    setCurrentItemByText(ui->cbSpeed, QString::number(settings.serailPortBaud()));

    ui->cbParity->setCurrentIndex(parityToIndex(settings.serialPortParity()));
    ui->cbStopBits->setCurrentIndex(stopBitsToIndex(settings.serialPortStopBits()));

    ui->sbAddress->setValue(settings.modbusSlaveAddress());
    ui->sbTimeOut->setValue(settings.modbusTimeout());
    ui->sbFrameDelay->setValue(settings.modbusFrameDelay());
    ui->sbRetries->setValue(settings.modbusRetries());
}

void SettingsDlg::storeSettings()
{
    Settings& settings = Settings::get();

    settings.setSerialPortName(ui->cbDevice->currentText());
    settings.setSerialPortBaud(ui->cbSpeed->currentText().toUInt());
    settings.setSerialPortParity(indexToParity(ui->cbParity->currentIndex()));
    settings.setSerialPortStopBits(indexToStopBits(ui->cbStopBits->currentIndex()));

    settings.setModbusSlaveAddress(ui->sbAddress->value());
    settings.setModbusTimeout(ui->sbTimeOut->value());
    settings.setModbusFrameDelay(ui->sbFrameDelay->value());
    settings.setModbusRetries(ui->sbRetries->value());
}

void SettingsDlg::populateDevicesList()
{
    auto portsInfo = QSerialPortInfo::availablePorts();

    for(const QSerialPortInfo& info: portsInfo){
        ui->cbDevice->addItem(info.portName());
    }

    QCompleter* completer = ui->cbDevice->completer();
    if(completer){
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setModelSorting(QCompleter::UnsortedModel);
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    }
}

void SettingsDlg::populateBaudsList()
{
    auto bauds = QSerialPortInfo::standardBaudRates();

    for(const qint32& baud: bauds){
        ui->cbSpeed->addItem(QString::number(baud));
    }

    QCompleter* completer = ui->cbSpeed->completer();
    if(completer){
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setModelSorting(QCompleter::UnsortedModel);
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    }
}

void SettingsDlg::setCurrentItemByText(QComboBox *combobox, const QString &text)
{
    int index = combobox->findText(text);
    if(index == -1){
        combobox->addItem(text);
        index = combobox->count() - 1;
    }
    combobox->setCurrentIndex(index);
}

int SettingsDlg::parityToIndex(QSerialPort::Parity parity)
{
    switch(parity){
    default:
    case QSerialPort::NoParity:
        return 0;
    case QSerialPort::EvenParity:
        return 1;
    case QSerialPort::OddParity:
        return 2;
    }
}

QSerialPort::Parity SettingsDlg::indexToParity(int index)
{
    switch(index){
    case 0:
    default:
        return QSerialPort::NoParity;
    case 1:
        return QSerialPort::EvenParity;
    case 2:
        return QSerialPort::OddParity;
    }
}

int SettingsDlg::stopBitsToIndex(QSerialPort::StopBits stopbits)
{
    switch(stopbits){
    default:
    case QSerialPort::OneStop:
        return 0;
    case QSerialPort::TwoStop:
        return 1;
    }
}

QSerialPort::StopBits SettingsDlg::indexToStopBits(int index)
{
    switch(index){
    default:
    case 0:
        return QSerialPort::OneStop;
    case 1:
        return QSerialPort::TwoStop;
    }
}
