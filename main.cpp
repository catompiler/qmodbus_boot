#include "mainwindow.h"
#include <QApplication>
#include "settings.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName(QStringLiteral("artem.lab"));
    a.setApplicationName(QStringLiteral("qmidbus_boot"));

    Settings::get().read();

    MainWindow w;
    w.show();

    int res = a.exec();

    Settings::get().write();

    return res;
}
