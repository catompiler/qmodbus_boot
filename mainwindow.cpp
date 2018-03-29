#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QByteArray>
#include <QDebug>
#include "settings.h"
#include "settingsdlg.h"
#include "modbusnet.h"
#include "modbusdev.h"
#include "modbusreg.h"
#include "modbusfile.h"
#include "modbusfirmware.h"


#define STATUSBAR_TIME 5000


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    settingsDlg = nullptr;

    modbus_net = new ModbusNet(this);
    modbus_dev = new ModbusDev(modbus_net, Settings::get().modbusSlaveAddress(), this);
    modbus_fw = new ModbusFirmware(modbus_dev);

    connect(modbus_net, &ModbusNet::errorOccured, this, &MainWindow::modbus_net_error_occured);
    connect(modbus_net, &ModbusNet::stateChanged, this, &MainWindow::modbus_net_state_changed);
    connect(modbus_net, &ModbusNet::connectedToNet, this, &MainWindow::connectedToNet);
    connect(modbus_net, &ModbusNet::disconnectedFromNet, this, &MainWindow::disconnectedFromNet);

    connect(modbus_fw, &ModbusFirmware::progressSetMin, ui->prbProgress, &QProgressBar::setMinimum);
    connect(modbus_fw, &ModbusFirmware::progressSetMax, ui->prbProgress, &QProgressBar::setMaximum);
    connect(modbus_fw, &ModbusFirmware::progressChanged, ui->prbProgress, &QProgressBar::setValue);

    connect(modbus_fw, &ModbusFirmware::confReaded, this, &MainWindow::confReaded);
    connect(modbus_fw, &ModbusFirmware::confReadErrorOccured, this, &MainWindow::confReadError);

    connect(modbus_fw, &ModbusFirmware::dataReaded, this, &MainWindow::readFlashDone);
    connect(modbus_fw, &ModbusFirmware::dataReadErrorOccured, this, &MainWindow::readFlashFail);
    connect(modbus_fw, &ModbusFirmware::dataReadCanceled, this, &MainWindow::readFlashCanceled);

    connect(modbus_fw, &ModbusFirmware::dataWrited, this, &MainWindow::writeFlashDone);
    connect(modbus_fw, &ModbusFirmware::dataWriteErrorOccured, this, &MainWindow::writeFlashFail);
    connect(modbus_fw, &ModbusFirmware::dataWriteCanceled, this, &MainWindow::writeFlashCanceled);

    modbus_net->setup();

    refreshUi();
}

MainWindow::~MainWindow()
{
    delete modbus_fw;
    delete modbus_dev;
    delete modbus_net;

    if(settingsDlg) delete settingsDlg;

    delete ui;
}

void MainWindow::modbus_net_state_changed(QModbusDevice::State state)
{
    QString stateStr;

    switch(state){
    case QModbusDevice::UnconnectedState:
        stateStr = tr("Разъединено");
        break;
    case QModbusDevice::ConnectingState:
        stateStr = tr("Соединение...");
        break;
    case QModbusDevice::ConnectedState:
        stateStr = tr("Соединено");
        break;
    case QModbusDevice::ClosingState:
        stateStr = tr("Разъединение...");
        break;
    default:
        break;
    }

    statusBar()->showMessage(stateStr, STATUSBAR_TIME);

    refreshUi();
}

void MainWindow::modbus_net_error_occured(ModbusErr error)
{
    statusBar()->showMessage(modbusErrorToString(error.modbusError()), STATUSBAR_TIME);

    refreshUi();
}

void MainWindow::refreshUi()
{
    bool connected = modbus_net->isConnectedToNet();
    bool fw_updated = modbus_fw->isConfReaded();
    bool fw_exec = modbus_fw->isExecuting();

    bool fw_ready = connected && fw_updated;

    ui->actSettings->setEnabled(!connected);
    ui->actConnect->setEnabled(!connected);
    ui->actDisconnect->setEnabled(connected);

    /*ui->leAddress->setEnabled(fw_ready && !fw_exec);
    ui->sbSize->setEnabled(fw_ready && !fw_exec);
    ui->pbSelectFile->setEnabled(fw_ready && !fw_exec);
    ui->leFileName->setEnabled(fw_ready && !fw_exec);*/

    ui->pbRead->setEnabled(fw_ready && !fw_exec);
    ui->pbWrite->setEnabled(fw_ready && !fw_exec);
    ui->pbCancel->setEnabled(fw_ready && fw_exec);

    ui->pbRun->setEnabled(fw_ready && !fw_exec);
}

QString MainWindow::modbusErrorToString(QModbusDevice::Error err) const
{
    QString res;

    switch(err){
    case QModbusDevice::NoError:
        res = tr("Нет ошибки");
        break;
    case QModbusDevice::ReadError:
        res = tr("Ошибка чтения");
        break;
    case QModbusDevice::WriteError:
        res = tr("Ошибка записи");
        break;
    case QModbusDevice::ConnectionError:
        res = tr("Ошибка соединения");
        break;
    case QModbusDevice::ConfigurationError:
        res = tr("Ошибка конфигурации");
        break;
    case QModbusDevice::TimeoutError:
        res = tr("Превышено время ожидания");
        break;
    case QModbusDevice::ProtocolError:
        res = tr("Ошибка протокола");
        break;
    case QModbusDevice::ReplyAbortedError:
        res = tr("Ответ отменён");
        break;
    default:
    case QModbusDevice::UnknownError:
        res = tr("Неизвестная ошибка");
        break;
    }

    return res;
}

QString MainWindow::makeErrorString(ModbusErr err) const
{
    QString res;

    res += tr("%1: %2")
            .arg(err.sanderName())
            .arg(err.errorStr());

    if(err.type() == ModbusErr::Modbus){
        res += tr("\nОшибка Modbus:\n"
                  "Код исключения: %1\n"
                  "Ошибка: \"%2\" (%3)\n"
                  "Текст ошибки: %4")
                .arg(static_cast<uint>(err.modbusException()))
                .arg(modbusErrorToString(err.modbusError()))
                .arg(static_cast<uint>(err.modbusError()))
                .arg(err.modbusErrorStr());
    }

    return res;
}

bool MainWindow::getAddrSize(quint32* address, quint32* size)
{
    int addr_base = 10;
    bool addr_ok = false;

    quint32 flash_addr = 0;
    quint32 flash_size = 0;

    if(address){
        QString addr_text = ui->leAddress->text();
        if(addr_text.startsWith(QStringLiteral("0x")) || addr_text.startsWith(QStringLiteral("0X"))){
            addr_base = 16;
        }else{
            addr_base = 10;
        }
        flash_addr = addr_text.toUInt(&addr_ok, addr_base);
        if(!addr_ok){
            QMessageBox::critical(this, tr("Ошибка"), tr("Неправильный адрес!"));
            return false;
        }
        *address = flash_addr;
    }

    if(size){
        flash_size = ui->sbSize->value();
        if(!flash_size){
            QMessageBox::critical(this, tr("Ошибка"), tr("Неправильный размер!"));
            return false;
        }
        *size = flash_size;
    }

    return true;
}

void MainWindow::on_actQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actSettings_triggered()
{
    if(!settingsDlg) settingsDlg = new SettingsDlg(this);

    settingsDlg->loadSettings();
    if(settingsDlg->exec()){
        settingsDlg->storeSettings();

        modbus_net->setup();

        Settings& settings = Settings::get();

        modbus_dev->setSlaveAddress(settings.modbusSlaveAddress());

        refreshUi();
    }
}

void MainWindow::on_actConnect_triggered()
{
    modbus_net->connectToNet();
}

void MainWindow::on_actDisconnect_triggered()
{
    modbus_net->disconnectFromNet();
}

void MainWindow::on_pbSelectFile_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Файл прошивки"),
                                                    QFileInfo(ui->leFileName->text()).absoluteFilePath(),
                                                    tr("Двоичный (*.bin);;Все файлы (*)"),
                                                    nullptr, QFileDialog::DontConfirmOverwrite);
    if(filename.isEmpty()) return;

    ui->leFileName->setText(filename);
}

void MainWindow::on_pbRead_clicked()
{
    if(ui->leFileName->text().isEmpty()){
        on_pbSelectFile_clicked();
        if(ui->leFileName->text().isEmpty()){
            QMessageBox::critical(this, tr("Чтение прошивки"), tr("Неправильное имя файла!"));
            return;
        }
    }

    quint32 flash_addr, flash_size;

    if(!getAddrSize(&flash_addr, &flash_size)) return;

    if(!modbus_fw->readData(flash_addr, flash_size)){
        QMessageBox::critical(this, tr("Чтение прошивки"), tr("Невозможно начать чтение!"));
        return;
    }

    refreshUi();
}

void MainWindow::on_pbWrite_clicked()
{
    if(ui->leFileName->text().isEmpty()){
        on_pbSelectFile_clicked();
        if(ui->leFileName->text().isEmpty()){
            QMessageBox::critical(this, tr("Запись прошивки"), tr("Неправильное имя файла!"));
            return;
        }
    }

    quint32 flash_addr;

    if(!getAddrSize(&flash_addr, nullptr)) return;

    QFile file(ui->leFileName->text());
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, tr("Запись прошивки"), tr("Невозможно открыть файл прошивки!"));
        return;
    }

    if(file.size() == 0){
        QMessageBox::critical(this, tr("Запись прошивки"), tr("Файл пуст!"));
        return;
    }

    QByteArray data = file.readAll();
    if(data.size() != file.size()){
        QMessageBox::critical(this, tr("Запись прошивки"), tr("Ошибка чтения файла прошивки!"));
        return;
    }

    if(!modbus_fw->writeData(flash_addr, data)){
        QMessageBox::critical(this, tr("Запись прошивки"), tr("Невозможно начать запись!"));
        return;
    }

    refreshUi();
}

void MainWindow::on_pbCancel_clicked()
{
    modbus_fw->cancel();
}

void MainWindow::on_pbRun_clicked()
{
    modbus_fw->runApp();
}

void MainWindow::confReaded()
{
    statusBar()->showMessage(tr("Объём памяти: %1 кбайт").arg(modbus_fw->flashSize()), STATUSBAR_TIME);
    refreshUi();
}

void MainWindow::confReadError(ModbusErr error)
{
    QMessageBox::critical(this, tr("Ошибка чтения конфигурации прошивки"), makeErrorString(error));

    refreshUi();
}

void MainWindow::readFlashDone()
{
    QFile file(ui->leFileName->text());
    if(!file.open(QIODevice::WriteOnly)){
        QMessageBox::critical(this, tr("Ошибка"), tr("Невозможно открыть файл для записи прошивки!"));
        return;
    }

    QByteArray data = modbus_fw->data();
    if(file.write(data.data(), data.size()) != data.size()){
        QMessageBox::critical(this, tr("Ошибка"), tr("Ошибка записи в файл прошивки!"));
        return;
    }

    QMessageBox::information(this, tr("Завершено"), tr("Прошивка успешно прочитана!"));

    refreshUi();
}

void MainWindow::readFlashFail(ModbusErr error)
{
    QMessageBox::critical(this, tr("Ошибка чтения"), makeErrorString(error));

    refreshUi();
}

void MainWindow::readFlashCanceled()
{
    QMessageBox::warning(this, tr("Отменено"), tr("Чтение прошивки было прекращено!"));

    refreshUi();
}

void MainWindow::writeFlashDone()
{
    QMessageBox::information(this, tr("Завершено"), tr("Прошивка успешно записана!"));

    refreshUi();
}

void MainWindow::writeFlashFail(ModbusErr error)
{
    QMessageBox::critical(this, tr("Ошибка записи"), makeErrorString(error));

    refreshUi();
}

void MainWindow::writeFlashCanceled()
{
    QMessageBox::warning(this, tr("Отменено"), tr("Запись прошивки была прекращена!"));

    refreshUi();
}

void MainWindow::connectedToNet()
{
    statusBar()->showMessage(tr("Чтение конфигурации памяти..."), STATUSBAR_TIME);

    modbus_fw->confRead();

    refreshUi();
}

void MainWindow::disconnectedFromNet()
{
    refreshUi();
}
