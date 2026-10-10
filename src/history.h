/*
 * Hex Calc - calculation history (~/.config/hex-calc/history.tsv).
 */
#pragma once

#include <QString>
#include <QVector>

struct HistoryEntry {
    qint64 time = 0;     // unix seconds
    int bits = 64;
    bool isSigned = true;
    int base = 16;
    quint64 value = 0;   // raw result bits
    QString expr;        // e.g. "1F × 2" as shown at the time
    QString result;      // e.g. "3E"
};

class History
{
public:
    static constexpr int Max = 500;

    explicit History(const QString &path);

    void load();
    bool save() const;

    // Appends an entry, dropping the oldest past Max. Returns how many old
    // entries were dropped.
    int add(const HistoryEntry &entry);
    void removeAt(int index);
    void clear();

    const QVector<HistoryEntry> &items() const { return m_items; }

private:
    QVector<HistoryEntry> m_items;  // oldest first
    QString m_path;
};
