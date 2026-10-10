/*
 * Hex Calc - colour themes, Qt palettes and style sheets.
 */
#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

#define THEME_FOLLOW_SYSTEM "follow-system"

struct Theme {
    const char *id;
    const char *name;
    bool dark;
    const char *bg;            // window background
    const char *surface;       // cards and panels
    const char *surfaceHi;     // digit keys, raised elements
    const char *fg;            // main text
    const char *fgDim;         // secondary text
    const char *primary;       // accent: '=' key, selection, set bits
    const char *onPrimary;
    const char *secondary;     // arithmetic operator keys
    const char *onSecondary;
    const char *tertiary;      // bitwise operator keys
    const char *onTertiary;
    const char *error;         // clear keys
    const char *onError;
    const char *danger;        // error text
    const char *outline;       // borders and separators
};

struct FontChoice {
    QString monoFamily;   // empty = system default font
    int monoSize = 30;
    QString uiFamily;     // empty = system default font
    int uiSize = 11;
};

struct StyleAssets {
    QString arrowDir;   // directory holding down-/up-<rgb>.png
    QString arrowTag;   // <rgb> part of the file names
};

namespace Themes {

int count();
const Theme &at(int index);
const Theme *find(const QString &id);
const Theme &fallback(bool dark);

QPalette palette(const Theme &t);
QString styleSheet(const Theme &t, const FontChoice &fonts, const StyleAssets &assets);

} // namespace Themes
