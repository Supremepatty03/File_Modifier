#include "config.h"
#include <QApplication>
#include <QFileDialog>
#include <QJsonDocument>
#include <QSettings>
#include <QJsonObject>


bool AppConfig::loadFromSettings(QSettings& settings) {
    inputDir = settings.value("inputDir").toString();
    fileMask = settings.value("fileMask").toString();
    deleteAfterProcess = settings.value("deleteAfterProcess").toBool();
    outputDir = settings.value("outputDir").toString();

    int cp = settings.value("collisionPolicy",
                            static_cast<int>(NameCollisionPolicy::Overwrite)).toInt();
    collisionPolicy = static_cast<NameCollisionPolicy>(cp);

    QString xorHex = settings.value("xorValue").toString();
    xorValue = QByteArray::fromHex(xorHex.toUtf8());

    int m = settings.value("mode",
                           static_cast<int>(RunMode::Single)).toInt();
    mode = static_cast<RunMode>(m);

    pollIntervalSec = settings.value("pollIntervalSec").toInt();
    return true;
}

bool AppConfig::saveToSettings(QSettings &settings) {
    settings.setValue("inputDir", inputDir);
    settings.setValue("fileMask", fileMask);
    settings.setValue("deleteAfterProcess", deleteAfterProcess);
    settings.setValue("outputDir", outputDir);
    settings.setValue("collisionPolicy", static_cast<int>(collisionPolicy));
    settings.setValue("xorValue", xorValue.toHex());
    settings.setValue("runMode", static_cast<int>(mode));
    settings.setValue("pollIntervalSec", pollIntervalSec);

    return true;
}
