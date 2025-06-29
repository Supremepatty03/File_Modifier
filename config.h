#ifndef CONFIG_H
#define CONFIG_H
#include <QApplication>
#include <QSettings>

enum class NameCollisionPolicy { Overwrite, AddCounter };
enum class RunMode { Single, Timer };

struct AppConfig {
    QString inputDir;
    QString fileMask;
    bool deleteAfterProcess;
    QString outputDir;
    NameCollisionPolicy collisionPolicy;
    QByteArray xorValue; // 8 байт
    RunMode mode;
    int pollIntervalSec;

    bool loadFromSettings(QSettings &settings);
    bool saveToSettings(QSettings &settings);
};

#endif // CONFIG_H
