#ifndef FILE_MODIFIER_H
#define FILE_MODIFIER_H
#include "config.h"
#include <QMutex>
class file_modifier : public QObject
{
    Q_OBJECT
public:
    explicit file_modifier(QObject* parent = nullptr);

public slots:
    void doProcessFiles();

    void cancel();

    void setParameters(const QString& inputDir,
                       const QString& fileMask,
                       const QString& outputDir,
                       const QByteArray& xorKey,
                       bool deleteOriginal,
                       NameCollisionPolicy policy);

signals:
    void logMessage(const QString& msg);
    void progress(int percent);
    void finished();

private:
    bool xorFile(const QString& inputPath, const QString& outputPath, const QByteArray& key);

    // Параметры
    QString m_inputDir;
    QString m_fileMask;
    QString m_outputDir;
    QByteArray m_xorKey;
    bool m_deleteOriginal;
    NameCollisionPolicy m_policy;
    QMutex m_logMutex;

    std::atomic<bool> m_abortRequested = false;
};



#endif // FILE_MODIFIER_H
