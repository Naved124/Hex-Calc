/*
 * Hex Calc - calculation history.
 *
 * One entry per line, tab separated:
 *   time  bits  signed  base  value(hex)  expression  result
 */
#include "history.h"

#include <QFile>
#include <QSaveFile>
#include <QStringList>
#include <QtGlobal>

History::History(const QString &path) : m_path(path) {}

static QString sanitize(QString s)
{
    s.replace(QLatin1Char('\t'), QLatin1Char(' '));
    s.replace(QLatin1Char('\n'), QLatin1Char(' '));
    s.replace(QLatin1Char('\r'), QLatin1Char(' '));
    return s;
}

void History::load()
{
    QFile f(m_path);
    m_items.clear();
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QList<QByteArray> lines = f.readAll().split('\n');
    for (const QByteArray &line : lines) {
        if (line.isEmpty())
            continue;
        const QStringList fields = QString::fromUtf8(line).split(QLatin1Char('\t'));
        if (fields.size() != 7)
            continue;

        HistoryEntry e;
        bool ok = true;
        e.time = fields[0].toLongLong();
        e.bits = fields[1].toInt();
        e.isSigned = fields[2].toInt() != 0;
        e.base = fields[3].toInt();
        e.value = fields[4].toULongLong(&ok, 16);
        e.expr = fields[5];
        e.result = fields[6];
        if (!ok)
            continue;
        if (e.bits != 8 && e.bits != 16 && e.bits != 32 && e.bits != 64)
            e.bits = 64;
        if (e.base != 2 && e.base != 8 && e.base != 10 && e.base != 16)
            e.base = 16;
        m_items.append(e);
    }
    while (m_items.size() > Max)
        m_items.removeFirst();
}

bool History::save() const
{
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("Could not save history to %s", qPrintable(m_path));
        return false;
    }
    for (const HistoryEntry &e : m_items) {
        const QString line = QStringLiteral("%1\t%2\t%3\t%4\t%5\t%6\t%7\n")
                                 .arg(e.time)
                                 .arg(e.bits)
                                 .arg(e.isSigned ? 1 : 0)
                                 .arg(e.base)
                                 .arg(e.value, 0, 16)
                                 .arg(e.expr, e.result);
        f.write(line.toUtf8());
    }
    return f.commit();
}

int History::add(const HistoryEntry &entry)
{
    HistoryEntry e = entry;
    int dropped = 0;

    e.expr = sanitize(e.expr);
    e.result = sanitize(e.result);
    m_items.append(e);
    while (m_items.size() > Max) {
        m_items.removeFirst();
        dropped++;
    }
    save();
    return dropped;
}

void History::removeAt(int index)
{
    if (index < 0 || index >= m_items.size())
        return;
    m_items.removeAt(index);
    save();
}

void History::clear()
{
    m_items.clear();
    save();
}
