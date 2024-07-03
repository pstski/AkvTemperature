#include "temperatureemulator.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TemperatureEmulator w;
    w.show();
    return a.exec();
}
