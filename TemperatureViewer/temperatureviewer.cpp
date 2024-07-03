#include "temperatureviewer.h"
#include "./ui_temperatureviewer.h"
#include <QDateTime>

const qint8 SECONDS_SHOW_ON_GRAPH = 30;  // Интервал отображаемых значений
QVector<double> time_axis;               // Значения времени
QVector<double> temperature_axis;        // Значения температуры
static qint64 temp_data_idx = 0;         // Количество измерений
static qint64 startTime;                 // Время начала работы

TemperatureViewer::TemperatureViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TemperatureViewer)
{
    ui->setupUi(this);

    /* Настройка отображения графика */
    ui->customPlot->addGraph();
    ui->customPlot->xAxis->setLabel("Время");
    ui->customPlot->yAxis->setLabel("Температура, °С");
    QCPTextElement *title = new QCPTextElement(ui->customPlot);
    title->setText("График температуры датчика в реальном времени");
    title->setFont(QFont("sans", 12, QFont::Bold));
    ui->customPlot->plotLayout()->insertRow(0);
    ui->customPlot->plotLayout()->addElement(0, 0, title);

    // Настройка линии
    QColor color(40, 110, 255);
    ui->customPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->customPlot->graph(0)->setPen(QPen(color));

    // Отображение шкалы времени
    QSharedPointer<QCPAxisTickerDateTime> date_time_ticker(new QCPAxisTickerDateTime);
    date_time_ticker->setDateTimeFormat("hh:mm:ss");
    ui->customPlot->xAxis->setTicker(date_time_ticker);

    // Настройка диапазона отображения значений
    startTime = QDateTime::currentSecsSinceEpoch();
    ui->customPlot->xAxis->setRange(startTime, startTime+SECONDS_SHOW_ON_GRAPH);
    ui->customPlot->yAxis->setRange(-4, 65);

    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    /* Сканирование доступных COM-портов */
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort port;
        port.setPort(info);
        if (port.open(QIODevice::ReadWrite))
        {
            ui->cb_COM->addItem(port.portName()); // Добавление доступных COM-портов в comboBox
        }
    }
}

/* Обработка нажатия на кнопку Подключиться/Отключиться */
void TemperatureViewer::on_pb_ConnectDisconnect_clicked()
{
    double now;
    /* Открытие COM-порта */
    if (connect_status == false)
    {
        m_serial.setBaudRate(QSerialPort::Baud9600);
        m_serial.setDataBits(QSerialPort::Data8);
        m_serial.setParity(QSerialPort::NoParity);
        m_serial.setStopBits(QSerialPort::OneStop);
        m_serial.setFlowControl(QSerialPort::NoFlowControl);
        m_serial.setPortName(ui->cb_COM->currentText());

        if (m_serial.open(QIODevice::ReadWrite))        // Если удалось открыть COM-порт

        {
            connect_status = true;
            connect(&m_serial, SIGNAL(readyRead()), this, SLOT(serialReceive()));

            // настройка GUI
            ui->pb_ConnectDisconnect->setText("Отключиться");
            ui->cb_COM->setDisabled(true);
            ui->pb_Control->setDisabled(true);
            ui->statusConnection->setText("Подключено");
            QPalette pal = ui->statusConnection->palette();
            pal.setColor(QPalette::Button, QColor(Qt::green));
            pal.setColor(QPalette::ButtonText, QColor(Qt::black));
            ui->statusConnection->setFlat(true);
            ui->statusConnection->setAutoFillBackground(true);
            ui->statusConnection->setPalette(pal);
            ui->statusConnection->update();

            // Настройка отображения графика
            startTime = QDateTime::currentSecsSinceEpoch();
            double now;
            now = startTime;
            ui->customPlot->xAxis->setRange( now, now+SECONDS_SHOW_ON_GRAPH);
            ui->customPlot->yAxis->setRange(-4, 65);

            // Настройка таймера запроса данных
            timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(sendRequest()) );
            timer->start(1000);

        }
        else      // Если  не удалось открыть COM-порт
        {
            QMessageBox::critical(this, "Ошибка", "Невозможно открыть выбранный COM-порт");
        }
    }
    /* Закртытие COM-порта */
    else
    {
        timer->stop();                                          // Остановка таймера запроса данных
        m_serial.close();
        disconnect(&m_serial, SIGNAL(readyRead()), 0,0);
        connect_status = m_serial.isOpen();

        // Настройка GUI
        ui->pb_ConnectDisconnect->setText("Подключиться");
        ui->cb_COM->setEnabled(true);
        ui->pb_Control->setEnabled(true);
        ui->statusConnection->setText("Отключено");
        ui->pb_ConnectDisconnect->setDisabled(true);

        QPalette pal = ui->statusConnection->palette();
        pal.setColor(QPalette::Button, QColor(Qt::red));
        pal.setColor(QPalette::ButtonText, QColor(Qt::black));
        ui->statusConnection->setFlat(true);
        ui->statusConnection->setAutoFillBackground(true);
        ui->statusConnection->setPalette(pal);
        ui->statusConnection->update();
    }

}

/* Функция отправки запроса данных по таймеру timer */
void TemperatureViewer::sendRequest()
{
    QByteArray sendData(1, 0x01);
    m_serial.write(sendData);

    // Запуск таймера ожидания ответа
    timerWait = new QTimer(this);
    connect(timerWait,SIGNAL(timeout()),this,SLOT(waitResponse()));
    timerWait->start(800);
}

/* Функция обработки приема данных*/
void TemperatureViewer::serialReceive()
{
    QByteArray data;
    data = m_serial.readAll();
    timerWait->stop();

    float temp_now = hexToFloat(QByteArray::fromHex(data));         // Перевод принятых hex данных в float
    double now = QDateTime::currentSecsSinceEpoch();                // Время приема данных
    time_axis.append(now);
    temperature_axis.append(temp_now);

    // Настройка отображения графика
    ui->customPlot->graph()->setData(time_axis,temperature_axis);
    if( ((qint64)(now) - startTime) > SECONDS_SHOW_ON_GRAPH )
    {
        ui->customPlot->xAxis->setRange(now, SECONDS_SHOW_ON_GRAPH, Qt::AlignRight);
    }
    ui->customPlot->replot();
    temp_data_idx++;
}

/* обработка нажатия на кнопку "Отправить контроль" */
void TemperatureViewer::on_pb_Control_clicked()
{
    if (m_serial.isOpen())                          // Проверка открыт ли COM-порт
    {
        m_serial.close();
    }
    // Открытие COM-порта
    m_serial.setBaudRate(QSerialPort::Baud9600);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);
    m_serial.setPortName(ui->cb_COM->currentText());
    if (m_serial.open(QIODevice::ReadWrite))
    {
        connect_status = true;
        connect(&m_serial, SIGNAL(readyRead()), this, SLOT(serialCheck()));
        ui->cb_COM->setDisabled(true);

        // Отправка команды "Контроль"
        QByteArray sendData(1, 0x00);
        m_serial.write(sendData);

        // Запуск таймера ожидания ответа
        timerCheck = new QTimer(this);
        connect(timerCheck,SIGNAL(timeout()),this,SLOT(waitCheck()));
        timerCheck->start(50);
    }
    else
    {
        QMessageBox::critical(this, "Ошибка", "Невозможно открыть выбранный COM-порт");
    }
}

/* Обработка ответа на команду "Контроль" */
void TemperatureViewer::serialCheck()
{
    QByteArray data;
    data = m_serial.readAll();

    if (data.toHex() == "00")            // Если принята команда ОК
    {
        ui->statusControl->setText("ОК");
        QPalette pal = ui->statusControl->palette();
        pal.setColor(QPalette::Button, QColor(Qt::green));
        pal.setColor(QPalette::ButtonText, QColor(Qt::black));
        ui->statusControl->setFlat(true);
        ui->statusControl->setAutoFillBackground(true);
        ui->statusControl->setPalette(pal);
        ui->statusControl->update();
        checked = true;
        ui->pb_ConnectDisconnect->setEnabled(true);
    }
    else if (data.toHex() == "01")        // Если принята команда НЕ ОК
    {
        ui->statusControl->setText("НЕ ОК");
        QPalette pal = ui->statusControl->palette();
        pal.setColor(QPalette::Button, QColor(Qt::red));
        pal.setColor(QPalette::ButtonText, QColor(Qt::black));
        ui->statusControl->setFlat(true);
        ui->statusControl->setAutoFillBackground(true);
        ui->statusControl->setPalette(pal);
        ui->statusControl->update();
        checked = true;
        ui->pb_ConnectDisconnect->setDisabled(true);
    }
}

// Обработка таймера ожидания ответа
void TemperatureViewer::waitCheck()
{
    timerCheck->stop();     // Остановка таймера

    if (checked == false)   // Если не принята команда от датчика
    {
        ui->statusControl->setText("НЕ ОК");
        QPalette pal = ui->statusControl->palette();
        pal.setColor(QPalette::Button, QColor(Qt::red));
        pal.setColor(QPalette::ButtonText, QColor(Qt::black));
        ui->statusControl->setFlat(true);
        ui->statusControl->setAutoFillBackground(true);
        ui->statusControl->setPalette(pal);
        ui->statusControl->update();
        ui->pb_ConnectDisconnect->setDisabled(true);
    }
    else checked = false;

    m_serial.close();
    connect_status = false;
    ui->cb_COM->setEnabled(true);
    disconnect(&m_serial, SIGNAL(readyRead()), 0,0);
}

/* Функция перевода hex в float */
float TemperatureViewer::hexToFloat(const QByteArray &hexData)
{
    QByteArray ba = QByteArray::fromHex(hexData);
    QDataStream dS (hexData);
    dS.setFloatingPointPrecision(QDataStream::SinglePrecision);
    double doubleValue;
    dS >> doubleValue;
    return doubleValue;
}

/* Если изменен COM-порт -> Connect(disabled) */
void TemperatureViewer::on_cb_COM_currentIndexChanged(int index)
{
    ui->pb_ConnectDisconnect->setDisabled(true);
}

void TemperatureViewer::waitResponse()
{
    timerWait->stop();
    timer->stop();

    QMessageBox::warning(this,"Ошибка", "Устройство отключено");

    on_pb_ConnectDisconnect_clicked();
    on_pb_Control_clicked();
}


TemperatureViewer::~TemperatureViewer()
{
    if (m_serial.isOpen())
    {
        m_serial.close();
    }
    delete ui;
}


