/*
 * Hex Calc - persistent preferences.
 */
#include "settings.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

QString Settings::configDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                  + QStringLiteral("/hex-calc");
    QDir().mkpath(dir);
    return dir;
}

static QString iniPath()
{
    return Settings::configDir() + QStringLiteral("/settings.ini");
}

void Settings::load()
{
    QSettings s(iniPath(), QSettings::IniFormat);

    s.beginGroup(QStringLiteral("Appearance"));
    theme = s.value(QStringLiteral("theme"), theme).toString();
    displayFont = s.value(QStringLiteral("display-font"), displayFont).toString();
    displaySize = qBound(12, s.value(QStringLiteral("display-size"), displaySize).toInt(), 72);
    uiFont = s.value(QStringLiteral("ui-font"), uiFont).toString();
    uiSize = qBound(8, s.value(QStringLiteral("ui-size"), uiSize).toInt(), 24);
    s.endGroup();

    s.beginGroup(QStringLiteral("Calculator"));
    base = s.value(QStringLiteral("base"), base).toInt();
    if (base != 2 && base != 8 && base != 10 && base != 16)
        base = 16;
    bits = s.value(QStringLiteral("bits"), bits).toInt();
    if (bits != 8 && bits != 16 && bits != 32 && bits != 64)
        bits = 64;
    isSigned = s.value(QStringLiteral("signed"), isSigned).toBool();
    groupDigits = s.value(QStringLiteral("group-digits"), groupDigits).toBool();
    showHistory = s.value(QStringLiteral("show-history"), showHistory).toBool();
    showBits = s.value(QStringLiteral("show-bits"), showBits).toBool();
    s.endGroup();

    s.beginGroup(QStringLiteral("Window"));
    geometry = s.value(QStringLiteral("geometry")).toByteArray();
    s.endGroup();
}

void Settings::save() const
{
    QSettings s(iniPath(), QSettings::IniFormat);

    s.beginGroup(QStringLiteral("Appearance"));
    s.setValue(QStringLiteral("theme"), theme);
    s.setValue(QStringLiteral("display-font"), displayFont);
    s.setValue(QStringLiteral("display-size"), displaySize);
    s.setValue(QStringLiteral("ui-font"), uiFont);
    s.setValue(QStringLiteral("ui-size"), uiSize);
    s.endGroup();

    s.beginGroup(QStringLiteral("Calculator"));
    s.setValue(QStringLiteral("base"), base);
    s.setValue(QStringLiteral("bits"), bits);
    s.setValue(QStringLiteral("signed"), isSigned);
    s.setValue(QStringLiteral("group-digits"), groupDigits);
    s.setValue(QStringLiteral("show-history"), showHistory);
    s.setValue(QStringLiteral("show-bits"), showBits);
    s.endGroup();

    s.beginGroup(QStringLiteral("Window"));
    s.setValue(QStringLiteral("geometry"), geometry);
    s.endGroup();
}
