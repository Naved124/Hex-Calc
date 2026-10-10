/*
 * Hex Calc - main window.
 */
#pragma once

#include "engine.h"
#include "history.h"
#include "settings.h"
#include "themes.h"

#include <QMainWindow>
#include <QPointer>

class QAction;
class QActionGroup;
class QFrame;
class QGraphicsOpacityEffect;
class QLabel;
class QListWidget;
class QPropertyAnimation;
class QPushButton;
class QStackedWidget;
class QTimer;
class QToolButton;
class PreferencesDialog;

enum class KeyKind { Digit, BinOp, UnOp, Equals, AllClear, ClearEntry, Back };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString &dataDir, bool systemDark, const QString &themeOverride,
               QWidget *parent = nullptr);
    ~MainWindow() override;

    const Settings &settings() const { return m_set; }

    // Used by the preferences dialog.
    void setThemeId(const QString &id);
    void setFontFamily(bool display, const QString &family);
    void setFontSize(bool display, int size);
    void setGroupDigits(bool on);
    void setSystemDark(bool dark);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void closeEvent(QCloseEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;

private:
    QWidget *buildHeader();
    QWidget *buildDisplay();
    QWidget *buildBases();
    QWidget *buildBits();
    QWidget *buildChips();
    QWidget *buildKeypad();
    QWidget *buildHistory();
    void buildActions();
    void buildToast();

    const Theme &currentTheme() const;
    void applyAppearance();
    void refresh();

    void press(KeyKind kind, int arg);
    bool handleKey(QKeyEvent *ev);
    QPushButton *buttonFor(KeyKind kind, int arg) const;
    void flash(QPushButton *button);

    void setBase(Base base);
    void setBits(int bits);
    void setSigned(bool isSigned);

    void showHistory(bool show, bool resizeWindow);
    void recordHistory(const EngineResult &r);
    void insertHistoryRow(int row, const HistoryEntry &e);
    void rebuildHistory();
    void updateHistoryEmpty();
    int entryIndexForRow(int row) const;
    void useHistoryRow(int row);
    void historyContextMenu(const QPoint &pos);

    QString format(uint64_t value, Base base, bool group) const;
    void copyText(const QString &text, const QString &what);
    void copyValue();
    void pasteValue();
    void toast(const QString &message);

    void showPreferences();
    void showShortcuts();
    void showAbout();

    Engine m_eng;
    Settings m_set;
    History m_hist;
    QString m_dataDir;
    bool m_systemDark;
    bool m_updating = false;

    QLabel *m_modeLabel = nullptr;
    QLabel *m_memLabel = nullptr;
    QLabel *m_exprLabel = nullptr;
    QLabel *m_valueLabel = nullptr;
    QPushButton *m_memBtns[5] = {};
    QToolButton *m_copyBtn = nullptr;
    QToolButton *m_pasteBtn = nullptr;
    QToolButton *m_menuBtn = nullptr;
    QToolButton *m_historyBtn = nullptr;
    QToolButton *m_clearHistoryBtn = nullptr;

    QFrame *m_baseRows[4] = {};
    QLabel *m_baseNames[4] = {};
    QLabel *m_baseValues[4] = {};
    QToolButton *m_baseCopy[4] = {};

    QWidget *m_bitsCard = nullptr;
    QWidget *m_bitRows[2] = {};
    QPushButton *m_bitBtns[64] = {};

    QPushButton *m_sizeBtns[4] = {};
    QPushButton *m_signBtns[2] = {};

    QPushButton *m_digitBtns[16] = {};
    QPushButton *m_binopBtns[OP_COUNT] = {};
    QPushButton *m_unopBtns[UOP_COUNT] = {};
    QPushButton *m_eqBtn = nullptr;
    QPushButton *m_acBtn = nullptr;
    QPushButton *m_ceBtn = nullptr;
    QPushButton *m_backBtn = nullptr;

    QWidget *m_historyWrap = nullptr;
    QStackedWidget *m_historyStack = nullptr;
    QListWidget *m_historyList = nullptr;

    QLabel *m_toast = nullptr;
    QGraphicsOpacityEffect *m_toastEffect = nullptr;
    QPropertyAnimation *m_toastAnim = nullptr;
    QTimer *m_toastTimer = nullptr;

    QAction *m_actHistory = nullptr;
    QAction *m_actBits = nullptr;
    QAction *m_actGroup = nullptr;
    QAction *m_actSigned = nullptr;
    QActionGroup *m_themeGroup = nullptr;

    QPointer<PreferencesDialog> m_prefs;
};
