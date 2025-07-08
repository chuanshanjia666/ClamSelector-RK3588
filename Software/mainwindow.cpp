#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_decodeThread(new DecodeThread(this))
{
    ui->setupUi(this);

    // 连接信号槽
    connect(ui->start_stop_button, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onManualIntervene);
    connect(m_decodeThread, &DecodeThread::frameDecoded, this, &MainWindow::displayFrame);
}

MainWindow::~MainWindow()
{
    delete ui;
    m_decodeThread->stopDecode();
}

void MainWindow::onStartStopClicked()
{
    if (!m_isDecoding)
    {
        // 开始解码
        ui->start_stop_button->setText("停止");
        // m_decodeThread->startDecode("rtsp://admin618@192.168.137.160:8554/video"); // 替换为你的视频文件路径
        m_decodeThread->startDecode("test.mp4"); // 替换为你的视频文件路径
        m_isDecoding = true;
    }
    else
    {

        ui->start_stop_button->setText("开始");
        m_decodeThread->stopDecode();
        m_isDecoding = false;
    }
}

void MainWindow::onManualIntervene()
{
    qDebug() << "人工介入按钮被点击！";
    // TODO: 实现人工干预逻辑
}

void MainWindow::displayFrame(const QImage &frame)
{
    // 显示帧到QLabel
    ui->video_label->setPixmap(QPixmap::fromImage(frame).scaled(
        ui->video_label->width(),
        ui->video_label->height(),
        Qt::KeepAspectRatio));
}
