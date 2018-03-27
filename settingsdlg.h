#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QDialog>
#include <QSerialPort>

namespace Ui {
class SettingsDlg;
}

class QComboBox;

class SettingsDlg : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDlg(QWidget *parent = 0);
    ~SettingsDlg();

    void loadSettings();
    void storeSettings();

private:
    Ui::SettingsDlg *ui;

    void populateDevicesList();
    void populateBaudsList();

    static void setCurrentItemByText(QComboBox* combobox, const QString& text);

    static int parityToIndex(QSerialPort::Parity parity);
    static QSerialPort::Parity indexToParity(int index);

    static int stopBitsToIndex(QSerialPort::StopBits stopbits);
    static QSerialPort::StopBits indexToStopBits(int index);
};

#endif // SETTINGSDLG_H
