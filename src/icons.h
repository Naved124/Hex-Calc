/*
 * Hex Calc - small line icons drawn in code, so they follow the theme
 * colours and don't depend on the desktop's icon theme.
 */
#pragma once

#include <QColor>
#include <QIcon>

enum class IconKind { Menu, History, Copy, Paste, Trash };

QIcon makeIcon(IconKind kind, const QColor &color);

// Writes small chevron images (down-<rgb>.png, up-<rgb>.png and @2x
// variants) for style sheets to reference. Returns the directory, or an
// empty string on failure.
QString writeArrowImages(const QColor &color);
