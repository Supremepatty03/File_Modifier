#include "file_modifier.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QtConcurrent>
#include <QFile>
#include <QMutex>

file_modifier::file_modifier(QObject* parent)
    : QObject(parent),
    m_deleteOriginal(false),
    m_policy(NameCollisionPolicy::Overwrite)
{}

void file_modifier::setParameters(const QString& inputDir, const QString& fileMask, const QString& outputDir, const QByteArray& xorKey, bool deleteOriginal, NameCollisionPolicy policy)
{
    m_inputDir = inputDir;
    m_fileMask = fileMask;
    m_outputDir = outputDir;
    m_xorKey = xorKey;
    m_deleteOriginal = deleteOriginal;
    m_policy = policy;
}

bool file_modifier::xorFile(const QString& inputPath,
                            const QString& outputPath,
                            const QByteArray& key)
{
    QFile inFile(inputPath);
    if (!inFile.open(QIODevice::ReadOnly)) {
        QMutexLocker locker(&m_logMutex);
        emit logMessage("❌ Не удалось открыть для чтения: " + inputPath);
        return false;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMutexLocker locker(&m_logMutex);
        emit logMessage("❌ Не удалось открыть для записи: " + outputPath);
        return false;
    }

    const int bufferSize = 4096;
    while (!inFile.atEnd()) {
        QByteArray buffer = inFile.read(bufferSize);
        if (buffer.isEmpty() && !inFile.atEnd()) {
            QMutexLocker locker(&m_logMutex);
            emit logMessage("❌ Ошибка чтения: " + inputPath);
            return false;
        }

        for (int i = 0; i < buffer.size(); ++i)
            buffer[i] ^= key[i % key.size()];

        if (outFile.write(buffer) == -1) {
            QMutexLocker locker(&m_logMutex);
            emit logMessage("❌ Ошибка записи: " + outputPath);
            return false;
        }
    }
    return true;
}

void file_modifier::doProcessFiles()
{
    m_abortRequested = false;
    QDir inDir(m_inputDir);
    QDir outDir(m_outputDir);

    // собираем имена
    QStringList masks = m_fileMask.split(';', Qt::SkipEmptyParts);
    for (QString& m : masks) m = m.trimmed();

    QStringList names;
    for (const QString& m : masks)
        names += inDir.entryList({m}, QDir::Files | QDir::NoDotAndDotDot);
    names.removeDuplicates();

    if (names.isEmpty()) {
        emit logMessage("⚠️ Нет файлов для обработки.");
        emit finished();
        return;
    }

    int total = names.size(), done = 0;
    for (const QString& name : names) {
        if (m_abortRequested == true){
            QMutexLocker locker(&m_logMutex);
            emit logMessage("⏹️ Обработка прервана пользователем.");
            break;
        }
        QString inPath  = inDir.filePath(name);
        QFileInfo fi(name);
        QString base = fi.completeBaseName();
        QString ext  = fi.suffix();
        QString outName = name;

        if (m_policy == NameCollisionPolicy::AddCounter) {
            int cnt = 1;
            while (QFile::exists(outDir.filePath(outName))) {
                outName = QString("%1_%2.%3").arg(base).arg(cnt++).arg(ext);
            }
        }

        QString outPath = outDir.filePath(outName);

        {
            QMutexLocker locker(&m_logMutex);
            emit logMessage(QString("🔄 %1 → %2").arg(inPath, outPath));
        }

        if (!xorFile(inPath, outPath, m_xorKey)) {
        } else if (m_deleteOriginal) {
            if (QFile::remove(inPath)) {
                QMutexLocker locker(&m_logMutex);
                emit logMessage("Удалён: " + inPath);
            }
        }

        done++;
        emit progress(done * 100 / total);
    }

    emit logMessage("✅ Обработка завершена.");
    emit finished();
}
void file_modifier::cancel(){
    m_abortRequested = true;
}
