#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQueue> // 引入队列
#include "decodethread.h"
#include "inferencethread.h"
#include"serialworker.h"
QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStopClicked();
    void onManualIntervene();
    void displayFrame(const QImage &frame);
    void handleInferenceResult(int classId, float confidence);
    void onSerialStartReceived();

private:
    Ui::MainWindow *ui;
    DecodeThread *m_decodeThread;
    bool m_isDecoding = false;
    InferenceThread *m_inferenceThread;
    QQueue<int> m_classQueue; // 用于存储类别 ID
    SerialWorker *m_serialWorker = nullptr;
};
#endif // MAINWINDOW_H