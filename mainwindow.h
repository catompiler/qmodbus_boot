#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "modbusnet.h"
#include "modbuserr.h"

class SettingsDlg;
class ModbusDev;
class ModbusReg;
class ModbusFirmware;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void modbus_net_state_changed(QModbusDevice::State state);
    void modbus_net_error_occured(ModbusErr error);
    void on_actQuit_triggered();
    void on_actSettings_triggered();
    void on_actConnect_triggered();
    void on_actDisconnect_triggered();

    void on_pbSelectFile_clicked();
    void on_pbRead_clicked();
    void on_pbWrite_clicked();
    void on_pbCancel_clicked();
    void on_pbRun_clicked();

    void confReaded();
    void confReadError(ModbusErr error);

    void readFlashDone();
    void readFlashFail(ModbusErr error);
    void readFlashCanceled();

    void writeFlashDone();
    void writeFlashFail(ModbusErr error);
    void writeFlashCanceled();

    void connectedToNet();
    void disconnectedFromNet();
private:
    void refreshUi();

    QString modbusErrorToString(QModbusDevice::Error err) const;
    QString makeErrorString(ModbusErr err) const;

    bool getAddrSize(quint32* address, quint32* size);

    Ui::MainWindow *ui;
    SettingsDlg* settingsDlg;
    ModbusNet* modbus_net;
    ModbusDev* modbus_dev;
    ModbusFirmware* modbus_fw;
};

#endif // MAINWINDOW_H
