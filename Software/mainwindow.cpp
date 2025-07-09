#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_decodeThread(new DecodeThread(this)), m_inferenceThread(new InferenceThread(this)),m_serialWorker(new SerialWorker("/dev/ttyS9", 9600, this))
{
    ui->setupUi(this);

    // 连接信号槽
    connect(ui->start_stop_button, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onManualIntervene);
    connect(m_decodeThread, &DecodeThread::frameDecodedForDisplay, this, &MainWindow::displayFrame);
    // 新增：连接解码线程到推理线程
    connect(m_decodeThread, &DecodeThread::frameDecoded,
            m_inferenceThread, &InferenceThread::enqueueFrame,
            Qt::QueuedConnection);
    connect(m_inferenceThread, &InferenceThread::inferenceResultReady,
            this, &MainWindow::handleInferenceResult,
            Qt::QueuedConnection);
    connect(m_serialWorker, &SerialWorker::startCommandReceived, this, &MainWindow::onSerialStartReceived);
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
        m_decodeThread->startDecode("rtsp://admin618@192.168.137.160:8554/video"); // 替换为你的视频文件路径
        // m_decodeThread->startDecode("test.mp4"); // 替换为你的视频文件路径
        // m_decodeThread->startDecode("/dev/video22"); // 替换为你的视频文件路径
        m_isDecoding = true;
        m_inferenceThread->start();
        m_serialWorker->start();
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

void MainWindow::handleInferenceResult(int classId, float confidence)
{
    // qDebug() << "📥 推理结果: 类别 =" << classId << ", 置信度 =" << confidence;
    // QString resultStr = QString("Class: %1, Confidence: %2").arg(classId).arg(confidence);
    m_classQueue.enqueue(classId);

    // 如果队列大小超过 10，出队一个元素
    if (m_classQueue.size() > 10)
    {
        int removedClassId = m_classQueue.dequeue(); // 出队
        qDebug() << "[Queue] Removed class ID:" << removedClassId;
    }
    ui->label_4->setText(QString("种类: %1").arg(classId));               // 显示类别
    ui->confidence_label->setText(QString("置信度: %1").arg(confidence)); // 显示置信度
}
void MainWindow::onSerialStartReceived()
{
    if (m_classQueue.isEmpty())
        return;

    QMap<int, int> countMap;
    for (int id : m_classQueue)
        countMap[id]++;

    int maxClass = -1, maxCount = -1;
    for (auto it = countMap.begin(); it != countMap.end(); ++it)
    {
        if (it.value() > maxCount)
        {
            maxClass = it.key();
            maxCount = it.value();
        }
    }

    qDebug() << "[主线程统计] 最常见类别:" << maxClass;

    if (m_serialWorker)
        m_serialWorker->sendClassResponse(maxClass); // 直接调用槽函数
}