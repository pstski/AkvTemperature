#ifndef TEMPERATUREEMULATOR_H
#define TEMPERATUREEMULATOR_H

#include <QMainWindow>
#include <QtSerialPort>
#include <QTime>
#include <QByteArray>
#include <QDataStream>

QT_BEGIN_NAMESPACE
namespace Ui {
class TemperatureEmulator;
}
QT_END_NAMESPACE

class TemperatureEmulator : public QMainWindow
{
    Q_OBJECT

public:
    TemperatureEmulator(QWidget *parent = nullptr);
    ~TemperatureEmulator();

private slots:
    void on_cbCOM_currentTextChanged(const QString &arg1);

    void on_rndButton_clicked();

    void serialRecieve(); // Прием данных

private:
    Ui::TemperatureEmulator *ui;

    QSerialPort m_serial;

    bool status;
};
#endif // TEMPERATUREEMULATOR_H
