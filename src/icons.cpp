/*
 * Hex Calc - small line icons drawn in code.
 */
#include "icons.h"

#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QStandardPaths>

static QPixmap render(IconKind kind, const QColor &color, int size, qreal dpr)
{
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 20.0, size / 20.0);  // draw on a 20x20 grid
    QPen pen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (kind) {
    case IconKind::Menu:
        p.drawLine(QPointF(3.5, 5.5), QPointF(16.5, 5.5));
        p.drawLine(QPointF(3.5, 10), QPointF(16.5, 10));
        p.drawLine(QPointF(3.5, 14.5), QPointF(16.5, 14.5));
        break;
    case IconKind::History:
        p.drawEllipse(QPointF(10, 10), 7, 7);
        p.drawLine(QPointF(10, 10), QPointF(10, 5.8));
        p.drawLine(QPointF(10, 10), QPointF(13, 12));
        break;
    case IconKind::Copy: {
        QRectF back(7, 3, 9.5, 11), front(3.5, 6, 9.5, 11);
        p.drawRoundedRect(back, 2, 2);
        // Clear the overlap so the back sheet looks hidden behind the front.
        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.setBrush(Qt::black);
        p.drawRoundedRect(front.adjusted(-0.8, -0.8, 0.8, 0.8), 2.5, 2.5);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        p.drawRoundedRect(front, 2, 2);
        break;
    }
    case IconKind::Paste:
        p.drawRoundedRect(QRectF(4, 4, 12, 13.5), 2, 2);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(7.25, 2.5, 5.5, 3), 1, 1);
        p.setBrush(Qt::NoBrush);
        p.drawLine(QPointF(7, 10), QPointF(13, 10));
        p.drawLine(QPointF(7, 13.5), QPointF(11, 13.5));
        break;
    case IconKind::Trash: {
        p.drawLine(QPointF(3.5, 5.5), QPointF(16.5, 5.5));
        p.drawLine(QPointF(8, 3.2), QPointF(12, 3.2));
        QPainterPath body;
        body.moveTo(5.2, 5.5);
        body.lineTo(6.2, 16.8);
        body.lineTo(13.8, 16.8);
        body.lineTo(14.8, 5.5);
        p.drawPath(body);
        p.drawLine(QPointF(8.5, 8.5), QPointF(8.7, 14));
        p.drawLine(QPointF(11.5, 8.5), QPointF(11.3, 14));
        break;
    }
    }
    return pm;
}

QIcon makeIcon(IconKind kind, const QColor &color)
{
    QIcon icon;
    for (int size : { 16, 20, 24 }) {
        icon.addPixmap(render(kind, color, size, 1.0));
        icon.addPixmap(render(kind, color, size, 2.0));
    }
    return icon;
}

QString writeArrowImages(const QColor &color)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty() || !QDir().mkpath(dir))
        return QString();
    const QString tag = color.name(QColor::HexRgb).mid(1);

    for (int up = 0; up < 2; up++) {
        for (int scale = 1; scale <= 2; scale++) {
            const QString path = QStringLiteral("%1/%2-%3%4.png")
                                     .arg(dir, up ? QStringLiteral("up") : QStringLiteral("down"), tag,
                                          scale == 2 ? QStringLiteral("@2x") : QString());
            if (QFile::exists(path))
                continue;
            QPixmap pm(10 * scale, 10 * scale);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);
            p.scale(scale, scale);
            p.setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            if (up)
                p.drawPolyline(QPolygonF({ QPointF(1.5, 6.5), QPointF(5, 3), QPointF(8.5, 6.5) }));
            else
                p.drawPolyline(QPolygonF({ QPointF(1.5, 3.5), QPointF(5, 7), QPointF(8.5, 3.5) }));
            p.end();
            pm.save(path, "PNG");
        }
    }
    return dir;
}
