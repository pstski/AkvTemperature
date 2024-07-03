#include "temperatureemulator.h"
#include "./ui_temperatureemulator.h"

TemperatureEmulator::TemperatureEmulator(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TemperatureEmulator)
{
    ui->setupUi(this);

    ui->textEdit->append("<font color=cyan>" + QTime::currentTime().toString() + " : Старт" + "</font>");

    // Проверка доступных COM-портов
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort port;
        port.setPort(info);
        if (port.open(QIODevice::ReadWrite))
        {
            ui->cbCOM->addItem(info.portName());
        }
    }
}

// Обработка поступающих команд
void TemperatureEmulator::serialRecieve()
{
    QByteArray data;
    data = m_serial.readAll();
    ui->textEdit->append("<font color=red>" + QTime::currentTime().toString() + " : Принято : 0x" + data.toHex() + "</font>");

    if (data.toHex() =="00" && status)              // Команда Контроль + состояние устройства ОК -> OK
    {
        QByteArray sendData(1, 0x00);
        m_serial.write(sendData);
        ui->textEdit->append("<font color=cyan>" + QTime::currentTime().toString() + " : " + "Отправлено >> 0x" + sendData.toHex() + "</font>");
    }
    else if (data.toHex() =="00" && !status)        // Команда Контроль + состояние устройства НЕ ОК -> НЕ OK
    {
        QByteArray sendData(1, 0x01);
        m_serial.write(sendData);
        ui->textEdit->append("<font color=cyan>" + QTime::currentTime().toString() + " : " + "Отправлено >> 0x" + sendData.toHex() + "</font>");

    }
    else if (data.toHex() =="01" && status)        // Команда ЗАПРОС + состояние устройства ОК -> значение температуры
    {
        QByteArray ba;
        QDataStream dS(&ba, QIODevice::WriteOnly);
        dS.setFloatingPointPrecision(QDataStream::SinglePrecision);

        if(ui->rndButton->isChecked())              // рандомные значения
        {
            double temp = QRandomGenerator::global()->bounded(-4,65);
            dS << temp;
        }
        else                                        // заданные значения
        {
        dS << ui->sbTEMP->value();
        }

        // передача данных
        QByteArray foo = (ba.toHex().toUpper());
        m_serial.write(foo);
        ui->textEdit->append("<font color=cyan>" + QTime::currentTime().toString() + " : " + "Отправлено >> 0x" + foo + "</font>");
    }
}

// При изменении COM-порта происходит автоматическое открытие выбранного порта
void TemperatureEmulator::on_cbCOM_currentTextChanged(const QString &arg1)
{
    if (m_serial.isOpen())
    {
        m_serial.close();
        disconnect(&m_serial, SIGNAL(readyRead()),0,0);
        ui->textEdit->append("<font color=cyan>" + QTime::currentTime().toString() + " : " + "Закрыт порт " + m_serial.portName() + "</font>");
    }

    m_serial.setPortName(ui->cbCOM->currentText());
    m_serial.setBaudRate(QSerialPort::Baud9600);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);
    if (m_serial.open(QIODevice::ReadWrite))
    {
        ui->textEdit->append("<font color=cyan>" + QTime::currentTime().toString() + " : " + "Открыт порт " + arg1 + "</font>");
        connect(&m_serial, SIGNAL(readyRead()), this, SLOT(serialRecieve()));
        status = true;
    }
    else
    {
        ui->textEdit->append("<font color=cyan>" + QTime::currentTime().toString() + " : " + "Не удалось открыть порт " + arg1 + "</font>");
        status = false;
    }
}

void TemperatureEmulator::on_rndButton_clicked()
{
    ui->sbTEMP->setDisabled(ui->rndButton->isChecked());
}

TemperatureEmulator::~TemperatureEmulator()
{
    delete ui;
    m_serial.close();
}


