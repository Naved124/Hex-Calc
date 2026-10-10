/*
 * Hex Calc - preferences dialog. Changes apply immediately.
 */
#pragma once

#include <QDialog>

class MainWindow;
class QCheckBox;
class QComboBox;
class QFontComboBox;
class QSpinBox;

// Bundled font families (shipped in the data directory's fonts/ folder).
extern const char *const BUNDLED_MONO_FONTS[];
extern const char *const BUNDLED_UI_FONTS[];

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(MainWindow *window);

    // Re-reads the window's settings (e.g. after a change from the menu).
    void syncFromSettings();

private:
    struct FontRow {
        QComboBox *preset = nullptr;
        QFontComboBox *any = nullptr;
        QSpinBox *size = nullptr;
    };

    void addFontSection(class QGridLayout *grid, int &row, bool display);
    void syncFontRow(bool display);

    MainWindow *m_window;
    QComboBox *m_theme = nullptr;
    QCheckBox *m_group = nullptr;
    FontRow m_rows[2];
    bool m_syncing = false;
};
