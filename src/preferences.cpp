/*
 * Hex Calc - preferences dialog.
 */
#include "preferences.h"

#include "mainwindow.h"
#include "settings.h"
#include "themes.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>

const char *const BUNDLED_MONO_FONTS[] = {
    "JetBrains Mono", "Fira Code", "Roboto Mono", "Source Code Pro", "Ubuntu Mono", nullptr,
};
const char *const BUNDLED_UI_FONTS[] = {
    "Roboto", "Inter", nullptr,
};

static QLabel *label(const QString &text, const char *name)
{
    auto *l = new QLabel(text);
    l->setObjectName(QLatin1String(name));
    return l;
}

PreferencesDialog::PreferencesDialog(MainWindow *window)
    : QDialog(window), m_window(window)
{
    setWindowTitle(QStringLiteral("Preferences"));

    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(24, 20, 24, 20);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(1, 1);
    int row = 0;

    // Appearance.
    m_theme = new QComboBox;
    m_theme->addItem(QStringLiteral("Follow system (Material light/dark)"),
                     QStringLiteral(THEME_FOLLOW_SYSTEM));
    for (int i = 0; i < Themes::count(); i++)
        m_theme->addItem(QLatin1String(Themes::at(i).name), QLatin1String(Themes::at(i).id));
    connect(m_theme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_syncing)
            m_window->setThemeId(m_theme->currentData().toString());
    });
    grid->addWidget(label(QStringLiteral("Appearance"), "sectionLabel"), row++, 0, 1, 2);
    grid->addWidget(label(QStringLiteral("Theme"), "dimLabel"), row, 0);
    grid->addWidget(m_theme, row++, 1);

    // Fonts.
    addFontSection(grid, row, true);
    addFontSection(grid, row, false);

    // Calculator.
    m_group = new QCheckBox(QStringLiteral("Group digits (DEAD BEEF, 1,000,000)"));
    connect(m_group, &QCheckBox::toggled, this, [this](bool on) {
        if (!m_syncing)
            m_window->setGroupDigits(on);
    });
    grid->addWidget(label(QStringLiteral("Calculator"), "sectionLabel"), row++, 0, 1, 2);
    grid->addWidget(m_group, row++, 0, 1, 2);

    auto *preview = label(QStringLiteral("DEAD BEEF  0x1F  1,234,567"), "preview");
    preview->setAlignment(Qt::AlignCenter);
    grid->addWidget(preview, row++, 0, 1, 2);

    syncFromSettings();
    setMinimumWidth(480);
}

void PreferencesDialog::addFontSection(QGridLayout *grid, int &row, bool display)
{
    FontRow &r = m_rows[display ? 0 : 1];

    r.preset = new QComboBox;
    if (display) {
        for (const char *const *f = BUNDLED_MONO_FONTS; *f; f++)
            r.preset->addItem(QLatin1String(*f), QLatin1String(*f));
        r.preset->addItem(QStringLiteral("System monospace"), QStringLiteral(FONT_SYSTEM_MONO));
        r.preset->addItem(QStringLiteral("System default"), QStringLiteral(FONT_SYSTEM_DEFAULT));
    } else {
        for (const char *const *f = BUNDLED_UI_FONTS; *f; f++)
            r.preset->addItem(QLatin1String(*f), QLatin1String(*f));
        r.preset->addItem(QStringLiteral("System default"), QStringLiteral(FONT_SYSTEM_DEFAULT));
        for (const char *const *f = BUNDLED_MONO_FONTS; *f; f++)
            r.preset->addItem(QLatin1String(*f), QLatin1String(*f));
    }

    r.any = new QFontComboBox;
    r.any->setToolTip(QStringLiteral("Pick any font installed on your system"));
    if (display)
        r.any->setFontFilters(QFontComboBox::AllFonts);

    r.size = new QSpinBox;
    r.size->setRange(display ? 16 : 8, display ? 56 : 18);
    r.size->setSuffix(QStringLiteral(" pt"));

    connect(r.preset, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, display](int) {
                if (m_syncing)
                    return;
                m_window->setFontFamily(display, m_rows[display ? 0 : 1].preset->currentData().toString());
                syncFontRow(display);
            });
    connect(r.any, &QFontComboBox::currentFontChanged, this, [this, display](const QFont &f) {
        if (m_syncing)
            return;
        m_window->setFontFamily(display, f.family());
        syncFontRow(display);
    });
    connect(r.size, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, display](int v) {
        if (!m_syncing)
            m_window->setFontSize(display, v);
    });

    grid->addWidget(label(display ? QStringLiteral("Display font") : QStringLiteral("Interface font"),
                          "sectionLabel"), row++, 0, 1, 2);
    grid->addWidget(label(QStringLiteral("Font"), "dimLabel"), row, 0);
    grid->addWidget(r.preset, row++, 1);
    grid->addWidget(label(QStringLiteral("Any system font"), "dimLabel"), row, 0);
    grid->addWidget(r.any, row++, 1);
    grid->addWidget(label(QStringLiteral("Size"), "dimLabel"), row, 0);
    grid->addWidget(r.size, row++, 1, Qt::AlignLeft);
}

void PreferencesDialog::syncFontRow(bool display)
{
    const Settings &s = m_window->settings();
    FontRow &r = m_rows[display ? 0 : 1];
    const QString family = display ? s.displayFont : s.uiFont;
    const bool old = m_syncing;
    m_syncing = true;

    int index = r.preset->findData(family);
    if (index < 0) {
        r.preset->addItem(family, family);
        index = r.preset->count() - 1;
    }
    r.preset->setCurrentIndex(index);

    QString resolved = family;
    if (family == QLatin1String(FONT_SYSTEM_MONO))
        resolved = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    else if (family == QLatin1String(FONT_SYSTEM_DEFAULT))
        resolved = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    r.any->setCurrentFont(QFont(resolved));

    r.size->setValue(display ? s.displaySize : s.uiSize);
    m_syncing = old;
}

void PreferencesDialog::syncFromSettings()
{
    const Settings &s = m_window->settings();
    m_syncing = true;
    const int index = m_theme->findData(s.theme);
    if (index >= 0)
        m_theme->setCurrentIndex(index);
    m_group->setChecked(s.groupDigits);
    m_syncing = false;
    syncFontRow(true);
    syncFontRow(false);
}
