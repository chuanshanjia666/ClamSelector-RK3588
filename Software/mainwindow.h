#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "decodethread.h"

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

private:
    Ui::MainWindow *ui;
    DecodeThread *m_decodeThread;
    bool m_isDecoding = false;
};
#endif // MAINWINDOW_H