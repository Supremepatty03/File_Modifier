#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QCloseEvent>
#include <QTimer>
#include <QSettings>
#include "config.h"
#include "file_modifier.h"

#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->StatusLineEdit->setText("Ожидание запуска ...");
    ui->StatusLineEdit->setStyleSheet("color: red;");
    loadSettings();
    connect(ui->InputDirButton, &QPushButton::clicked, this, &MainWindow::on_InputDirButton);
    connect(ui->OutputDirButton, &QPushButton::clicked, this, &MainWindow::on_OutputDirButton);
    connect(ui->StartButton, &QPushButton::clicked, this, &MainWindow::on_StartButton);
    connect(ui->StopButton, &QPushButton::clicked, this, &MainWindow::on_StopButton);

    pollingTimer = new QTimer(this);
    connect(pollingTimer, &QTimer::timeout, this, &MainWindow::onTimerTriggered);
    initComboBoxes();
}

void MainWindow::onTimerTriggered(){
    on_StartButton();
}
void MainWindow::initComboBoxes() {
    ui->NamesCollisionComboBox->addItem("Перезаписать");
    ui->NamesCollisionComboBox->addItem("Добавить индекс");

    ui->WorkModeComboBox->addItem("Один запуск");
    ui->WorkModeComboBox->addItem("Работа по таймеру");
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_InputDirButton() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите входную папку"),
                                                    QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        ui->EntryDirLineEdit->setText(dir);
    }
    else {ui->EntryDirLineEdit->setText("Ошибка!");}
}
void MainWindow::on_OutputDirButton() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите папку вывода"),
                                                    QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        ui->OutputDirLineEdit->setText(dir);
    }
    else {ui->EntryDirLineEdit->setText("Ошибка!");}
}
void MainWindow::loadSettings()
{
    QSettings settings("Supremepatty", "FileModifier");
    config.loadFromSettings(settings);
    ui->EntryDirLineEdit->setText(config.inputDir);
    ui->lineEdit_2->setText(config.fileMask);
    ui->DeleteAfterCheckBox->setChecked(config.deleteAfterProcess);
    ui->OutputDirLineEdit->setText(config.outputDir);

    ui->NamesCollisionComboBox->setCurrentIndex(
        config.collisionPolicy == NameCollisionPolicy::Overwrite ? 0 : 1
        );

    ui->XORLineEdit->setText(config.xorValue.toHex());

    ui->WorkModeComboBox->setCurrentIndex(
        config.mode == RunMode::Single ? 0 : 1
        );

    ui->IntervalSpinBox->setValue(config.pollIntervalSec);
}
void MainWindow::saveSettings()
{
    QSettings settings("Supremepatty", "FileModifier");

    config.inputDir = ui->EntryDirLineEdit->text();
    config.fileMask = ui->lineEdit_2->text();
    config.deleteAfterProcess = ui->DeleteAfterCheckBox->isChecked();
    config.outputDir = ui->OutputDirLineEdit->text();

    switch (ui->NamesCollisionComboBox->currentIndex()) {
    case 0:
        config.collisionPolicy = NameCollisionPolicy::Overwrite;
        break;
    case 1:
        config.collisionPolicy = NameCollisionPolicy::AddCounter;
        break;
    }

    QByteArray raw = QByteArray::fromHex(ui->XORLineEdit->text().toUtf8());
    if (raw.size() == 8)
        config.xorValue = raw;
    else
        config.xorValue = QByteArray(8, 0); // дефолт

    switch (ui->WorkModeComboBox->currentIndex()) {
    case 0:
        config.mode = RunMode::Single;
        break;
    case 1:
        config.mode = RunMode::Timer;
        break;
    }

    config.pollIntervalSec = ui->IntervalSpinBox->value();

    config.saveToSettings(settings);
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (pollingTimer->isActive())
        pollingTimer->stop();
    QMainWindow::closeEvent(event);
}
void MainWindow::on_StartButton()
{
    if (m_processingThread || (m_timer && m_timer->isActive())) {
        ui->LogEdit->append("⚠️ Уже идёт обработка. Сначала нажмите Стоп.");
        return;
    }

    const QString inputDir = ui->EntryDirLineEdit->text();
    const QString fileMask = ui->lineEdit_2->text();
    const QString outputDir = ui->OutputDirLineEdit->text();
    const QString hexStr = ui->XORLineEdit->text().trimmed();
    const bool deleteOrig = ui->DeleteAfterCheckBox->isChecked();
    const int intervalSec = ui->IntervalSpinBox->value();
    NameCollisionPolicy policy= ui->NamesCollisionComboBox->currentIndex() == 1 ? NameCollisionPolicy::AddCounter : NameCollisionPolicy::Overwrite;
    static const QRegularExpression re("^[0-9A-Fa-f]{16}$");
    if (!re.match(hexStr).hasMatch()) {
        ui->LogEdit->append("❌ Ошибка: XOR-ключ должен быть 16 hex-символов.");
        return;
    }
    QByteArray xorKey = QByteArray::fromHex(hexStr.toUtf8());
    RunMode mode = ui->WorkModeComboBox->currentIndex() == 1 ? RunMode::Timer : RunMode::Single;

    //создание worker+thread и запуск
    auto launchWorker = [=]() {
        // ожидание освобождения
        if (m_processingThread) return;

        m_modifier           = new file_modifier;
        m_processingThread = new QThread(this);
        m_modifier->moveToThread(m_processingThread);
        m_modifier->setParameters(inputDir,
                                fileMask,
                                outputDir,
                                xorKey,
                                deleteOrig,
                                policy);

        connect(m_processingThread, &QThread::started,
                m_modifier, &file_modifier::doProcessFiles);
        connect(m_modifier, &file_modifier::logMessage,
                this, [&](const QString& msg){
                    ui->LogEdit->append(msg);
                    ui->StatusLineEdit->setText(msg);
                });
        connect(m_modifier, &file_modifier::progress,
                ui->progressBar, &QProgressBar::setValue);
        connect(m_modifier, &file_modifier::finished, this, [&](){
            m_processingThread->quit();
        });
        connect(m_processingThread, &QThread::finished, this, [&](){
            m_modifier->deleteLater();
            m_processingThread->deleteLater();
            m_modifier = nullptr;
            m_processingThread = nullptr;
            ui->StatusLineEdit->setText("Готово");
            ui->StatusLineEdit->setStyleSheet("color: green;");
        });

        m_processingThread->start();
    };

    if (mode == RunMode::Single) {
        ui->LogEdit->append("Однократный запуск");
        ui->StatusLineEdit->setStyleSheet("color: blue;");
        launchWorker();
    } else {
        ui->LogEdit->append(
            QString("Режим таймера: каждые %1 с").arg(intervalSec));
        ui->StatusLineEdit->setStyleSheet("color: orange;");
        ui->StatusLineEdit->setText("Таймер активен");

        if (!m_timer) {
            m_timer = new QTimer(this);
            connect(m_timer, &QTimer::timeout, this, [=](){
                launchWorker();
            });
        }
        m_timer->start(intervalSec * 1000);
    }
}

void MainWindow::on_StopButton(){
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        ui->LogEdit->append("Таймер остановлен");
    }
    if(m_modifier){
        m_modifier->cancel();
        ui->LogEdit->append("Отправлен запрос на остановку.");
    }
}

