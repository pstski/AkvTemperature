#ifndef TEMPERATUREVIEWER_H
#define TEMPERATUREVIEWER_H

#include <QMainWindow>
#include <QTimer>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>

QT_BEGIN_NAMESPACE
namespace Ui {
class TemperatureViewer;
}
QT_END_NAMESPACE

class TemperatureViewer : public QMainWindow
{
    Q_OBJECT

public:
    TemperatureViewer(QWidget *parent = nullptr);
    ~TemperatureViewer();


private slots:
    void on_pb_ConnectDisconnect_clicked();     // Нажатие кнопки "Подключиться/Отключиться"

    void on_pb_Control_clicked();               // Нажатие кнопки "Отправить контроль"

    void sendRequest();                         // Отправка команды "Запрос"

    void serialReceive();                       // Прием ответа на команду "Запрос"

    void serialCheck();                         // Прием ответа на команду "Контроль"

    void waitCheck();                           // Обработка таймера ожидания команды "Контроль"

    void on_cb_COM_currentIndexChanged(int index);

    void waitResponse();

private:
    Ui::TemperatureViewer *ui;

    QTimer *timer;
    QTimer *timerCheck;
    QTimer *timerWait;

    bool checked = false;
    bool connect_status = false;

    QSerialPort m_serial;

    float hexToFloat(const QByteArray &hexData);
};
#endif // TEMPERATUREVIEWER_H
