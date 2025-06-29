#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include "config.h"
#include "file_modifier.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadSettings();
    void saveSettings();
    void closeEvent(QCloseEvent *event);
    void initComboBoxes();

public slots:
    void on_InputDirButton();
    void on_OutputDirButton();
    void on_StartButton();
    void onTimerTriggered();
    void on_StopButton();

private:
    Ui::MainWindow *ui;
    AppConfig config;
    QTimer *pollingTimer;
    file_modifier* m_modifier      = nullptr;
    QTimer* m_timer              = nullptr;
    QThread* m_processingThread   = nullptr;
};
#endif // MAINWINDOW_H
