/*
 * Hex Calc - persistent preferences (~/.config/hex-calc/settings.ini).
 */
#pragma once

#include <QByteArray>
#include <QString>

// Special font values.
#define FONT_SYSTEM_DEFAULT "system"
#define FONT_SYSTEM_MONO "monospace"

struct Settings {
    QString theme = QStringLiteral("material-dark");
    QString displayFont = QStringLiteral("JetBrains Mono");
    int displaySize = 30;
    QString uiFont = QStringLiteral("Roboto");
    int uiSize = 11;

    int base = 16;
    int bits = 64;
    bool isSigned = true;
    bool groupDigits = true;
    bool showHistory = false;
    bool showBits = true;

    QByteArray geometry;

    void load();
    void save() const;

    // Directory for settings and history; created if missing.
    static QString configDir();
};
