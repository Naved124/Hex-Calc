/*
 * Hex Calc - main window.
 */
#include "mainwindow.h"

#include "icons.h"
#include "preferences.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>

namespace {

constexpr int HistoryWidth = 300;
constexpr int HistoryGap = 12;

struct KeyDef {
    const char *label;
    KeyKind kind;
    int arg;
    const char *style;
    const char *tip;
};

#define DIGIT(l, n) { l, KeyKind::Digit, n, "digit", nullptr }

const KeyDef KEYPAD[6][6] = {
    {
        { "AND", KeyKind::BinOp, OP_AND, "bitop", "Bitwise AND  ( & )" },
        { "OR", KeyKind::BinOp, OP_OR, "bitop", "Bitwise OR  ( | )" },
        { "XOR", KeyKind::BinOp, OP_XOR, "bitop", "Bitwise XOR  ( ^ )" },
        { "NOT", KeyKind::UnOp, UOP_NOT, "bitop", "Bitwise NOT  ( ~ )" },
        { "CE", KeyKind::ClearEntry, 0, "clear", "Clear entry  (Delete)" },
        { "\xE2\x8C\xAB", KeyKind::Back, 0, "func", "Backspace" },
    },
    {
        { "NAND", KeyKind::BinOp, OP_NAND, "bitop", "Bitwise NAND" },
        { "NOR", KeyKind::BinOp, OP_NOR, "bitop", "Bitwise NOR" },
        { "<<", KeyKind::BinOp, OP_SHL, "bitop", "Shift left  ( < )" },
        { ">>", KeyKind::BinOp, OP_SHR, "bitop", "Shift right  ( > ), arithmetic when signed" },
        { "AC", KeyKind::AllClear, 0, "clear", "Clear all  (Esc)" },
        { "\xC3\xB7", KeyKind::BinOp, OP_DIV, "op", "Divide  ( / )" },
    },
    { DIGIT("A", 10), DIGIT("B", 11), DIGIT("7", 7), DIGIT("8", 8), DIGIT("9", 9),
      { "\xC3\x97", KeyKind::BinOp, OP_MUL, "op", "Multiply  ( * )" } },
    { DIGIT("C", 12), DIGIT("D", 13), DIGIT("4", 4), DIGIT("5", 5), DIGIT("6", 6),
      { "\xE2\x88\x92", KeyKind::BinOp, OP_SUB, "op", "Subtract  ( - )" } },
    { DIGIT("E", 14), DIGIT("F", 15), DIGIT("1", 1), DIGIT("2", 2), DIGIT("3", 3),
      { "+", KeyKind::BinOp, OP_ADD, "op", "Add  ( + )" } },
    {
        { "ROL", KeyKind::BinOp, OP_ROL, "bitop", "Rotate left" },
        { "ROR", KeyKind::BinOp, OP_ROR, "bitop", "Rotate right" },
        { "\xC2\xB1", KeyKind::UnOp, UOP_NEG, "func", "Negate, two's complement  (F9)" },
        DIGIT("0", 0),
        { "MOD", KeyKind::BinOp, OP_MOD, "op", "Remainder  ( % )" },
        { "=", KeyKind::Equals, 0, "equals", "Equals  (Enter)" },
    },
};

#undef DIGIT

const Base BASE_ORDER[4] = { BASE_HEX, BASE_DEC, BASE_OCT, BASE_BIN };
const int SIZES[4] = { 8, 16, 32, 64 };
const char *const SIZE_NAMES[4] = { "BYTE", "WORD", "DWORD", "QWORD" };
const char *const SIZE_KEYS[4] = { "F4", "F3", "F2", "F12" };

QString u8(const char *s)
{
    return QString::fromUtf8(s);
}

// Re-evaluates style sheet rules that depend on dynamic properties.
void repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

void setProp(QWidget *w, const char *name, const QVariant &value)
{
    if (w->property(name) == value)
        return;
    w->setProperty(name, value);
    repolish(w);
}

// Long digit strings without spaces would not wrap; zero-width spaces
// give the text layout places to break.
QString softWrap(const QString &s)
{
    if (s.contains(QLatin1Char(' ')) || s.size() <= 16)
        return s;
    QString out;
    for (int i = 0; i < s.size(); i++) {
        if (i > 0 && (s.size() - i) % 8 == 0)
            out += QChar(0x200B);
        out += s[i];
    }
    return out;
}

QLabel *makeLabel(const QString &text, const char *name, Qt::Alignment align = Qt::AlignLeft)
{
    auto *l = new QLabel(text);
    l->setObjectName(QLatin1String(name));
    l->setAlignment(align | Qt::AlignVCenter);
    return l;
}

QToolButton *makeToolButton(const QString &tip)
{
    auto *b = new QToolButton;
    b->setToolTip(tip);
    b->setFocusPolicy(Qt::NoFocus);
    b->setAutoRaise(true);
    b->setIconSize(QSize(18, 18));
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

// A frame that reacts to clicks (used for the base rows).
class ClickFrame : public QFrame
{
public:
    std::function<void()> onClick;

protected:
    void mouseReleaseEvent(QMouseEvent *ev) override
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QPoint pos = ev->position().toPoint();
#else
        const QPoint pos = ev->pos();
#endif
        if (ev->button() == Qt::LeftButton && rect().contains(pos) && onClick)
            onClick();
        QFrame::mouseReleaseEvent(ev);
    }
};

} // namespace

/* ------------------------------------------------------------------ */

MainWindow::MainWindow(const QString &dataDir, bool systemDark, const QString &themeOverride,
                       QWidget *parent)
    : QMainWindow(parent),
      m_hist(Settings::configDir() + QStringLiteral("/history.tsv")),
      m_dataDir(dataDir),
      m_systemDark(systemDark)
{
    m_set.load();
    if (!themeOverride.isEmpty()) {
        if (themeOverride == QLatin1String(THEME_FOLLOW_SYSTEM) || Themes::find(themeOverride))
            m_set.theme = themeOverride;
        else
            qWarning("Unknown theme '%s' (see --list-themes)", qPrintable(themeOverride));
    }
    if (m_set.theme != QLatin1String(THEME_FOLLOW_SYSTEM) && !Themes::find(m_set.theme))
        m_set.theme = QStringLiteral("material-dark");
    m_hist.load();

    engine_init(&m_eng);
    engine_set_bits(&m_eng, m_set.bits);
    engine_set_signed(&m_eng, m_set.isSigned);
    engine_set_base(&m_eng, Base(m_set.base));

    setWindowTitle(QStringLiteral("Hex Calc"));

    auto *central = new QWidget;
    central->setObjectName(QStringLiteral("central"));
    auto *outer = new QHBoxLayout(central);
    outer->setContentsMargins(12, 8, 12, 12);
    outer->setSpacing(0);

    auto *calc = new QVBoxLayout;
    calc->setSpacing(10);
    calc->addWidget(buildHeader());
    calc->addWidget(buildDisplay());
    calc->addWidget(buildBases());
    calc->addWidget(buildBits());
    calc->addWidget(buildChips());
    calc->addWidget(buildKeypad(), 1);

    outer->addLayout(calc, 1);
    outer->addWidget(buildHistory());
    setCentralWidget(central);

    buildToast();
    buildActions();
    applyAppearance();

    m_bitsCard->setVisible(m_set.showBits);
    m_historyWrap->setVisible(m_set.showHistory);

    if (m_set.geometry.isEmpty() || !restoreGeometry(m_set.geometry))
        resize(m_set.showHistory ? 500 + HistoryWidth + HistoryGap : 500, sizeHint().height());

    qApp->installEventFilter(this);
    refresh();
}

MainWindow::~MainWindow()
{
    qApp->removeEventFilter(this);
}

/* ------------------------------------------------------------------ */
/* Construction                                                        */

QWidget *MainWindow::buildHeader()
{
    auto *w = new QWidget;
    auto *row = new QHBoxLayout(w);
    row->setContentsMargins(4, 0, 0, 0);
    row->setSpacing(4);

    m_modeLabel = makeLabel(QString(), "modeLabel");
    m_historyBtn = makeToolButton(QStringLiteral("History  (Ctrl+H)"));
    m_menuBtn = makeToolButton(QStringLiteral("Menu"));
    m_menuBtn->setPopupMode(QToolButton::InstantPopup);

    row->addWidget(m_modeLabel, 1);
    row->addWidget(m_historyBtn);
    row->addWidget(m_menuBtn);
    return w;
}

QWidget *MainWindow::buildDisplay()
{
    static const char *const memLabels[5] = { "MC", "MR", "M+", "M\xE2\x88\x92", "MS" };
    static const char *const memTips[5] = {
        "Memory clear", "Memory recall", "Add to memory", "Subtract from memory", "Memory store"
    };

    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("card"));
    auto *box = new QVBoxLayout(card);
    box->setContentsMargins(16, 10, 12, 6);
    box->setSpacing(2);

    m_exprLabel = makeLabel(QString(), "exprLabel", Qt::AlignRight);
    m_exprLabel->setWordWrap(true);

    m_valueLabel = makeLabel(QStringLiteral("0"), "valueLabel", Qt::AlignRight);
    m_valueLabel->setWordWrap(true);
    m_valueLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    auto *bottom = new QHBoxLayout;
    bottom->setSpacing(2);
    for (int i = 0; i < 5; i++) {
        auto *b = new QPushButton(u8(memLabels[i]));
        b->setProperty("kind", QStringLiteral("mem"));
        b->setFocusPolicy(Qt::NoFocus);
        b->setToolTip(QLatin1String(memTips[i]));
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, i] {
            switch (i) {
            case 0: engine_mem_clear(&m_eng); break;
            case 1: engine_mem_recall(&m_eng); break;
            case 2: engine_mem_add(&m_eng); break;
            case 3: engine_mem_sub(&m_eng); break;
            case 4: engine_mem_store(&m_eng); break;
            }
            refresh();
        });
        bottom->addWidget(b);
        m_memBtns[i] = b;
    }
    m_memLabel = makeLabel(QStringLiteral("M"), "memIndicator", Qt::AlignCenter);
    bottom->addSpacing(6);
    bottom->addWidget(m_memLabel);
    bottom->addStretch(1);

    m_pasteBtn = makeToolButton(QStringLiteral("Paste  (Ctrl+V)"));
    m_copyBtn = makeToolButton(QStringLiteral("Copy  (Ctrl+C)"));
    connect(m_pasteBtn, &QToolButton::clicked, this, &MainWindow::pasteValue);
    connect(m_copyBtn, &QToolButton::clicked, this, &MainWindow::copyValue);
    bottom->addWidget(m_pasteBtn);
    bottom->addWidget(m_copyBtn);

    box->addWidget(m_exprLabel);
    box->addWidget(m_valueLabel);
    box->addSpacing(4);
    box->addLayout(bottom);
    return card;
}

QWidget *MainWindow::buildBases()
{
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("card"));
    auto *box = new QVBoxLayout(card);
    box->setContentsMargins(6, 6, 6, 6);
    box->setSpacing(0);

    for (int i = 0; i < 4; i++) {
        const Base base = BASE_ORDER[i];
        auto *row = new ClickFrame;
        row->setObjectName(QStringLiteral("baseRow"));
        row->setAttribute(Qt::WA_Hover);
        row->setCursor(Qt::PointingHandCursor);
        row->onClick = [this, base] { setBase(base); };

        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(10, 3, 4, 3);
        h->setSpacing(10);

        auto *name = makeLabel(QLatin1String(engine_base_name(base)), "baseName");
        name->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        auto *value = makeLabel(QStringLiteral("0"), "baseValue");
        value->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        value->setWordWrap(true);
        value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        auto *copy = makeToolButton(QStringLiteral("Copy %1 value").arg(QLatin1String(engine_base_name(base))));
        connect(copy, &QToolButton::clicked, this, [this, base] {
            copyText(format(m_eng.value, base, false),
                     QStringLiteral("%1 value").arg(QLatin1String(engine_base_name(base))));
        });

        h->addWidget(name, 0, Qt::AlignTop);
        h->addWidget(value, 1);
        h->addWidget(copy, 0, Qt::AlignTop);
        box->addWidget(row);

        m_baseRows[i] = row;
        m_baseNames[i] = name;
        m_baseValues[i] = value;
        m_baseCopy[i] = copy;
    }
    return card;
}

QWidget *MainWindow::buildBits()
{
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("card"));
    auto *box = new QVBoxLayout(card);
    box->setContentsMargins(10, 8, 10, 8);
    box->setSpacing(2);

    for (int r = 0; r < 2; r++) {
        const int hi = 63 - 32 * r;
        auto *roww = new QWidget;
        auto *row = new QHBoxLayout(roww);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(0);
        row->addStretch(1);

        auto *hiLabel = makeLabel(QString::number(hi), "bitIndex", Qt::AlignRight);
        hiLabel->setMinimumWidth(18);
        row->addWidget(hiLabel);
        row->addSpacing(6);

        for (int n = 0; n < 8; n++) {
            if (n > 0)
                row->addSpacing(n == 4 ? 10 : 5);
            for (int j = 0; j < 4; j++) {
                const int bit = hi - (n * 4 + j);
                auto *b = new QPushButton(QStringLiteral("0"));
                b->setProperty("kind", QStringLiteral("bit"));
                b->setProperty("on", false);
                b->setFocusPolicy(Qt::NoFocus);
                b->setToolTip(QStringLiteral("Bit %1").arg(bit));
                b->setCursor(Qt::PointingHandCursor);
                connect(b, &QPushButton::clicked, this, [this, bit] {
                    engine_toggle_bit(&m_eng, bit);
                    refresh();
                });
                row->addWidget(b);
                m_bitBtns[bit] = b;
            }
        }

        row->addSpacing(6);
        auto *loLabel = makeLabel(QString::number(hi - 31), "bitIndex");
        loLabel->setMinimumWidth(18);
        row->addWidget(loLabel);
        row->addStretch(1);

        box->addWidget(roww);
        m_bitRows[r] = roww;
    }
    m_bitsCard = card;
    return card;
}

QWidget *MainWindow::buildChips()
{
    auto *w = new QWidget;
    auto *row = new QHBoxLayout(w);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto *sizes = new QButtonGroup(this);
    for (int i = 0; i < 4; i++) {
        auto *b = new QPushButton(QString::number(SIZES[i]));
        b->setCheckable(true);
        b->setProperty("kind", QStringLiteral("chip"));
        b->setProperty("seg", i == 0 ? QStringLiteral("first") : i == 3 ? QStringLiteral("last")
                                                                        : QStringLiteral("mid"));
        b->setFocusPolicy(Qt::NoFocus);
        b->setCursor(Qt::PointingHandCursor);
        b->setToolTip(QStringLiteral("%1, %2-bit  (%3)")
                          .arg(QLatin1String(SIZE_NAMES[i])).arg(SIZES[i])
                          .arg(QLatin1String(SIZE_KEYS[i])));
        const int bits = SIZES[i];
        connect(b, &QPushButton::clicked, this, [this, bits] { setBits(bits); });
        sizes->addButton(b);
        row->addWidget(b);
        m_sizeBtns[i] = b;
    }
    row->addStretch(1);

    auto *signs = new QButtonGroup(this);
    const char *const signLabels[2] = { "Unsigned", "Signed" };
    for (int i = 0; i < 2; i++) {
        auto *b = new QPushButton(QLatin1String(signLabels[i]));
        b->setCheckable(true);
        b->setProperty("kind", QStringLiteral("chip"));
        b->setProperty("seg", i == 0 ? QStringLiteral("first") : QStringLiteral("last"));
        b->setFocusPolicy(Qt::NoFocus);
        b->setCursor(Qt::PointingHandCursor);
        b->setToolTip(QStringLiteral("Two's complement interpretation  (Ctrl+S)"));
        connect(b, &QPushButton::clicked, this, [this, i] { setSigned(i == 1); });
        signs->addButton(b);
        row->addWidget(b);
        m_signBtns[i] = b;
    }
    return w;
}

QWidget *MainWindow::buildKeypad()
{
    auto *w = new QWidget;
    auto *grid = new QGridLayout(w);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(6);

    for (int r = 0; r < 6; r++) {
        grid->setRowStretch(r, 1);
        for (int c = 0; c < 6; c++) {
            const KeyDef &k = KEYPAD[r][c];
            const QString label = u8(k.label);
            auto *b = new QPushButton(label);

            b->setProperty("kind", QLatin1String(k.style));
            // Single glyphs such as the division sign need a larger size.
            if (k.kind != KeyKind::Digit && label.size() == 1)
                b->setProperty("sym", true);
            b->setFocusPolicy(Qt::NoFocus);
            b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            b->setCursor(Qt::PointingHandCursor);
            if (k.tip)
                b->setToolTip(QLatin1String(k.tip));
            const KeyKind kind = k.kind;
            const int arg = k.arg;
            connect(b, &QPushButton::clicked, this, [this, kind, arg] { press(kind, arg); });
            grid->addWidget(b, r, c);

            switch (k.kind) {
            case KeyKind::Digit: m_digitBtns[k.arg] = b; break;
            case KeyKind::BinOp: m_binopBtns[k.arg] = b; break;
            case KeyKind::UnOp: m_unopBtns[k.arg] = b; break;
            case KeyKind::Equals: m_eqBtn = b; break;
            case KeyKind::AllClear: m_acBtn = b; break;
            case KeyKind::ClearEntry: m_ceBtn = b; break;
            case KeyKind::Back: m_backBtn = b; break;
            }
        }
    }
    for (int c = 0; c < 6; c++)
        grid->setColumnStretch(c, 1);
    return w;
}

QWidget *MainWindow::buildHistory()
{
    m_historyWrap = new QWidget;
    auto *wrap = new QHBoxLayout(m_historyWrap);
    wrap->setContentsMargins(HistoryGap, 0, 0, 0);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("historyPanel"));
    panel->setFixedWidth(HistoryWidth);
    wrap->addWidget(panel);

    auto *box = new QVBoxLayout(panel);
    box->setContentsMargins(6, 10, 6, 6);
    box->setSpacing(4);

    auto *header = new QHBoxLayout;
    header->setContentsMargins(10, 0, 2, 0);
    header->addWidget(makeLabel(QStringLiteral("History"), "historyTitle"), 1);
    m_clearHistoryBtn = makeToolButton(QStringLiteral("Clear history"));
    connect(m_clearHistoryBtn, &QToolButton::clicked, this, [this] {
        if (m_hist.items().isEmpty())
            return;
        m_hist.clear();
        rebuildHistory();
        toast(QStringLiteral("History cleared"));
    });
    header->addWidget(m_clearHistoryBtn);
    box->addLayout(header);

    auto *empty = makeLabel(QStringLiteral("No calculations yet.\nResults appear here when you press =."),
                            "emptyLabel", Qt::AlignCenter);
    empty->setWordWrap(true);

    m_historyList = new QListWidget;
    m_historyList->setSelectionMode(QAbstractItemView::NoSelection);
    m_historyList->setFocusPolicy(Qt::NoFocus);
    m_historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_historyList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_historyList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_historyList->setMouseTracking(true);
    connect(m_historyList, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *item) { useHistoryRow(m_historyList->row(item)); });
    connect(m_historyList, &QListWidget::customContextMenuRequested, this,
            &MainWindow::historyContextMenu);

    m_historyStack = new QStackedWidget;
    m_historyStack->addWidget(empty);
    m_historyStack->addWidget(m_historyList);
    box->addWidget(m_historyStack, 1);

    return m_historyWrap;
}

void MainWindow::buildToast()
{
    m_toast = new QLabel(centralWidget());
    m_toast->setObjectName(QStringLiteral("toast"));
    m_toast->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_toast->hide();

    m_toastEffect = new QGraphicsOpacityEffect(m_toast);
    m_toastEffect->setOpacity(0.0);
    m_toast->setGraphicsEffect(m_toastEffect);
    m_toastAnim = new QPropertyAnimation(m_toastEffect, "opacity", this);
    m_toastAnim->setDuration(160);

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, this, [this] {
        m_toastAnim->stop();
        m_toastAnim->setStartValue(m_toastEffect->opacity());
        m_toastAnim->setEndValue(0.0);
        m_toastAnim->start();
    });
    connect(m_toastAnim, &QPropertyAnimation::finished, this, [this] {
        if (m_toastEffect->opacity() <= 0.01)
            m_toast->hide();
    });
}

void MainWindow::buildActions()
{
    auto make = [this](const QString &text, const QKeySequence &key, bool checkable = false) {
        auto *a = new QAction(text, this);
        a->setShortcut(key);
        a->setShortcutContext(Qt::WindowShortcut);
        a->setCheckable(checkable);
        addAction(a);  // keeps the shortcut working while the menu is closed
        return a;
    };

    m_actHistory = make(QStringLiteral("History"), QKeySequence(QStringLiteral("Ctrl+H")), true);
    m_actHistory->setChecked(m_set.showHistory);
    m_actHistory->setToolTip(QStringLiteral("History  (Ctrl+H)"));
    connect(m_actHistory, &QAction::toggled, this, [this](bool on) {
        m_set.showHistory = on;
        m_set.save();
        showHistory(on, true);
    });
    m_historyBtn->setCheckable(true);
    m_historyBtn->setChecked(m_set.showHistory);
    connect(m_historyBtn, &QToolButton::toggled, m_actHistory, &QAction::setChecked);
    connect(m_actHistory, &QAction::toggled, m_historyBtn, &QToolButton::setChecked);

    m_actBits = make(QStringLiteral("Bit Grid"), QKeySequence(QStringLiteral("Ctrl+B")), true);
    m_actBits->setChecked(m_set.showBits);
    connect(m_actBits, &QAction::toggled, this, [this](bool on) {
        m_set.showBits = on;
        m_set.save();
        m_bitsCard->setVisible(on);
    });

    m_actGroup = make(QStringLiteral("Group Digits"), QKeySequence(QStringLiteral("Ctrl+G")), true);
    m_actGroup->setChecked(m_set.groupDigits);
    connect(m_actGroup, &QAction::toggled, this, [this](bool on) { setGroupDigits(on); });

    m_actSigned = make(QStringLiteral("Signed"), QKeySequence(QStringLiteral("Ctrl+S")), true);
    m_actSigned->setChecked(m_set.isSigned);
    connect(m_actSigned, &QAction::toggled, this, [this](bool on) {
        if (!m_updating)
            setSigned(on);
    });

    auto *copy = make(QStringLiteral("Copy"), QKeySequence::Copy);
    connect(copy, &QAction::triggered, this, &MainWindow::copyValue);
    auto *paste = make(QStringLiteral("Paste"), QKeySequence::Paste);
    connect(paste, &QAction::triggered, this, &MainWindow::pasteValue);

    auto *prefs = make(QStringLiteral("Preferences"), QKeySequence(QStringLiteral("Ctrl+,")));
    connect(prefs, &QAction::triggered, this, &MainWindow::showPreferences);
    auto *shortcuts = make(QStringLiteral("Keyboard Shortcuts"), QKeySequence(QStringLiteral("F1")));
    connect(shortcuts, &QAction::triggered, this, &MainWindow::showShortcuts);
    auto *about = make(QStringLiteral("About Hex Calc"), QKeySequence());
    connect(about, &QAction::triggered, this, &MainWindow::showAbout);
    auto *quit = make(QStringLiteral("Quit"), QKeySequence(QStringLiteral("Ctrl+Q")));
    connect(quit, &QAction::triggered, this, &MainWindow::close);

    // Theme submenu.
    auto *menu = new QMenu(this);
    auto *themes = menu->addMenu(QStringLiteral("Theme"));
    m_themeGroup = new QActionGroup(this);
    m_themeGroup->setExclusive(true);
    auto addTheme = [&](const QString &id, const QString &name) {
        auto *a = themes->addAction(name);
        a->setCheckable(true);
        a->setData(id);
        a->setChecked(id == m_set.theme);
        m_themeGroup->addAction(a);
    };
    addTheme(QStringLiteral(THEME_FOLLOW_SYSTEM), QStringLiteral("Follow system"));
    themes->addSeparator();
    for (int i = 0; i < Themes::count(); i++)
        addTheme(QLatin1String(Themes::at(i).id), QLatin1String(Themes::at(i).name));
    connect(m_themeGroup, &QActionGroup::triggered, this,
            [this](QAction *a) { setThemeId(a->data().toString()); });

    menu->addAction(prefs);
    menu->addSeparator();
    menu->addAction(m_actBits);
    menu->addAction(m_actHistory);
    menu->addAction(m_actGroup);
    menu->addAction(m_actSigned);
    menu->addSeparator();
    menu->addAction(shortcuts);
    menu->addAction(about);
    menu->addAction(quit);
    m_menuBtn->setMenu(menu);
}

/* ------------------------------------------------------------------ */
/* Appearance                                                          */

const Theme &MainWindow::currentTheme() const
{
    if (m_set.theme == QLatin1String(THEME_FOLLOW_SYSTEM))
        return Themes::fallback(m_systemDark);
    const Theme *t = Themes::find(m_set.theme);
    return t ? *t : Themes::fallback(true);
}

static QString resolveFamily(const QString &family, bool display)
{
    if (family == QLatin1String(FONT_SYSTEM_MONO))
        return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    if (family.isEmpty() || family == QLatin1String(FONT_SYSTEM_DEFAULT))
        return display ? QFontDatabase::systemFont(QFontDatabase::GeneralFont).family() : QString();
    return family;
}

void MainWindow::applyAppearance()
{
    const Theme &t = currentTheme();
    FontChoice fonts;
    fonts.monoFamily = resolveFamily(m_set.displayFont, true);
    fonts.monoSize = m_set.displaySize;
    fonts.uiFamily = resolveFamily(m_set.uiFont, false);
    fonts.uiSize = m_set.uiSize;

    const QColor fg(QLatin1String(t.fg)), dim(QLatin1String(t.fgDim));
    StyleAssets assets;
    assets.arrowDir = writeArrowImages(dim);
    assets.arrowTag = dim.name(QColor::HexRgb).mid(1);

    qApp->setPalette(Themes::palette(t));
    qApp->setStyleSheet(Themes::styleSheet(t, fonts, assets));

    m_menuBtn->setIcon(makeIcon(IconKind::Menu, fg));
    m_historyBtn->setIcon(makeIcon(IconKind::History, fg));
    m_copyBtn->setIcon(makeIcon(IconKind::Copy, dim));
    m_pasteBtn->setIcon(makeIcon(IconKind::Paste, dim));
    m_clearHistoryBtn->setIcon(makeIcon(IconKind::Trash, dim));
    for (QToolButton *b : m_baseCopy)
        b->setIcon(makeIcon(IconKind::Copy, dim));

    // Row heights depend on the fonts.
    rebuildHistory();
}

void MainWindow::setThemeId(const QString &id)
{
    if (id != QLatin1String(THEME_FOLLOW_SYSTEM) && !Themes::find(id))
        return;
    m_set.theme = id;
    m_set.save();
    for (QAction *a : m_themeGroup->actions())
        if (a->data().toString() == id)
            a->setChecked(true);
    applyAppearance();
    if (m_prefs)
        m_prefs->syncFromSettings();
}

void MainWindow::setFontFamily(bool display, const QString &family)
{
    QString &slot = display ? m_set.displayFont : m_set.uiFont;
    if (slot == family)
        return;
    slot = family;
    m_set.save();
    applyAppearance();
}

void MainWindow::setFontSize(bool display, int size)
{
    int &slot = display ? m_set.displaySize : m_set.uiSize;
    if (slot == size)
        return;
    slot = size;
    m_set.save();
    applyAppearance();
}

void MainWindow::setGroupDigits(bool on)
{
    m_set.groupDigits = on;
    m_set.save();
    if (m_actGroup->isChecked() != on)
        m_actGroup->setChecked(on);
    if (m_prefs)
        m_prefs->syncFromSettings();
    refresh();
}

void MainWindow::setSystemDark(bool dark)
{
    if (m_systemDark == dark)
        return;
    m_systemDark = dark;
    if (m_set.theme == QLatin1String(THEME_FOLLOW_SYSTEM))
        applyAppearance();
}

/* ------------------------------------------------------------------ */
/* Display                                                             */

QString MainWindow::format(uint64_t value, Base base, bool group) const
{
    char buf[ENGINE_FORMAT_MAX];
    engine_format(value, base, m_eng.bits, m_eng.is_signed, group, buf, sizeof buf);
    return QString::fromLatin1(buf);
}

void MainWindow::refresh()
{
    const Engine &e = m_eng;
    const bool group = m_set.groupDigits;
    m_updating = true;

    // Main value.
    if (e.error) {
        m_valueLabel->setText(QString::fromUtf8(e.message));
        setProp(m_valueLabel, "error", true);
        setProp(m_valueLabel, "len", QString());
    } else {
        const QString text = format(e.value, e.base, group);
        const int len = text.size();
        m_valueLabel->setText(softWrap(text));
        setProp(m_valueLabel, "error", false);
        setProp(m_valueLabel, "len", len > 30 ? QStringLiteral("xxl")
                                     : len > 22 ? QStringLiteral("xl")
                                     : len > 15 ? QStringLiteral("l") : QString());
    }

    char expr[ENGINE_EXPR_MAX];
    engine_expression(&e, group, expr, sizeof expr);
    // A non-breaking space keeps the line's height when there is no expression.
    m_exprLabel->setText(expr[0] ? QString::fromUtf8(expr) : QString(QChar(0x00A0)));

    m_modeLabel->setText(QStringLiteral("%1  ·  %2-BIT  ·  %3")
                             .arg(QLatin1String(engine_base_name(e.base)))
                             .arg(e.bits)
                             .arg(e.is_signed ? QStringLiteral("SIGNED") : QStringLiteral("UNSIGNED")));

    // Memory.
    m_memLabel->setVisible(e.has_memory);
    if (e.has_memory)
        m_memLabel->setToolTip(QStringLiteral("Memory: %1").arg(format(e.memory, e.base, group)));
    m_memBtns[0]->setEnabled(e.has_memory);
    m_memBtns[1]->setEnabled(e.has_memory);

    // Every base.
    for (int i = 0; i < 4; i++) {
        const QString text = format(e.value, BASE_ORDER[i], group);
        const bool active = BASE_ORDER[i] == e.base;
        m_baseValues[i]->setText(softWrap(text));
        setProp(m_baseValues[i], "long", text.size() > 40);
        setProp(m_baseRows[i], "active", active);
        setProp(m_baseNames[i], "active", active);
    }

    // Bit grid.
    for (int i = 0; i < 64; i++) {
        const bool on = (e.value >> i) & 1;
        m_bitBtns[i]->setText(on ? QStringLiteral("1") : QStringLiteral("0"));
        setProp(m_bitBtns[i], "on", on);
        m_bitBtns[i]->setEnabled(i < e.bits);
    }
    m_bitRows[0]->setVisible(e.bits == 64);

    // Toggles.
    for (int i = 0; i < 4; i++)
        m_sizeBtns[i]->setChecked(SIZES[i] == e.bits);
    m_signBtns[0]->setChecked(!e.is_signed);
    m_signBtns[1]->setChecked(e.is_signed);
    if (m_actSigned->isChecked() != e.is_signed)
        m_actSigned->setChecked(e.is_signed);

    // Digits that don't exist in this base.
    for (int d = 0; d < 16; d++)
        m_digitBtns[d]->setEnabled(engine_digit_valid(e.base, d));

    m_updating = false;
}

/* ------------------------------------------------------------------ */
/* Input                                                               */

void MainWindow::setBase(Base base)
{
    engine_set_base(&m_eng, base);
    m_set.base = int(base);
    m_set.save();
    refresh();
}

void MainWindow::setBits(int bits)
{
    engine_set_bits(&m_eng, bits);
    m_set.bits = m_eng.bits;
    m_set.save();
    refresh();
}

void MainWindow::setSigned(bool isSigned)
{
    engine_set_signed(&m_eng, isSigned);
    m_set.isSigned = isSigned;
    m_set.save();
    refresh();
}

QPushButton *MainWindow::buttonFor(KeyKind kind, int arg) const
{
    switch (kind) {
    case KeyKind::Digit: return (arg >= 0 && arg < 16) ? m_digitBtns[arg] : nullptr;
    case KeyKind::BinOp: return (arg > OP_NONE && arg < OP_COUNT) ? m_binopBtns[arg] : nullptr;
    case KeyKind::UnOp: return (arg >= 0 && arg < UOP_COUNT) ? m_unopBtns[arg] : nullptr;
    case KeyKind::Equals: return m_eqBtn;
    case KeyKind::AllClear: return m_acBtn;
    case KeyKind::ClearEntry: return m_ceBtn;
    case KeyKind::Back: return m_backBtn;
    }
    return nullptr;
}

void MainWindow::flash(QPushButton *button)
{
    if (!button || !button->isEnabled())
        return;
    setProp(button, "flash", true);
    QPointer<QPushButton> guard(button);
    QTimer::singleShot(110, this, [guard] {
        if (guard)
            setProp(guard, "flash", false);
    });
}

void MainWindow::press(KeyKind kind, int arg)
{
    EngineResult r;

    switch (kind) {
    case KeyKind::Digit:
        if (!engine_input_digit(&m_eng, arg))
            QApplication::beep();
        break;
    case KeyKind::BinOp:
        engine_binop(&m_eng, BinOp(arg));
        break;
    case KeyKind::UnOp:
        engine_unop(&m_eng, UnOp(arg));
        break;
    case KeyKind::Equals:
        if (engine_equals(&m_eng, m_set.groupDigits, &r))
            recordHistory(r);
        break;
    case KeyKind::AllClear:
        engine_clear(&m_eng);
        break;
    case KeyKind::ClearEntry:
        engine_clear_entry(&m_eng);
        break;
    case KeyKind::Back:
        engine_backspace(&m_eng);
        break;
    }
    refresh();
}

bool MainWindow::handleKey(QKeyEvent *ev)
{
    const Qt::KeyboardModifiers mods = ev->modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier);
    if (mods != Qt::NoModifier)
        return false;  // leave Ctrl/Alt combinations to the shortcuts

    KeyKind kind;
    int arg = 0;

    switch (ev->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: kind = KeyKind::Equals; break;
    case Qt::Key_Escape: kind = KeyKind::AllClear; break;
    case Qt::Key_Delete: kind = KeyKind::ClearEntry; break;
    case Qt::Key_Backspace: kind = KeyKind::Back; break;
    case Qt::Key_F9: kind = KeyKind::UnOp; arg = UOP_NEG; break;
    case Qt::Key_F5: setBase(BASE_HEX); return true;
    case Qt::Key_F6: setBase(BASE_DEC); return true;
    case Qt::Key_F7: setBase(BASE_OCT); return true;
    case Qt::Key_F8: setBase(BASE_BIN); return true;
    case Qt::Key_F12: setBits(64); return true;
    case Qt::Key_F2: setBits(32); return true;
    case Qt::Key_F3: setBits(16); return true;
    case Qt::Key_F4: setBits(8); return true;
    default: {
        const QString text = ev->text();
        if (text.size() != 1)
            return false;
        const QChar c = text[0];
        const char ch = c.toLatin1();
        if (ch >= '0' && ch <= '9') {
            kind = KeyKind::Digit;
            arg = ch - '0';
        } else if ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
            kind = KeyKind::Digit;
            arg = (ch | 0x20) - 'a' + 10;
        } else {
            kind = KeyKind::BinOp;
            switch (ch) {
            case '+': arg = OP_ADD; break;
            case '-': arg = OP_SUB; break;
            case '*': case 'x': case 'X': arg = OP_MUL; break;
            case '/': arg = OP_DIV; break;
            case '%': arg = OP_MOD; break;
            case '&': arg = OP_AND; break;
            case '|': arg = OP_OR; break;
            case '^': arg = OP_XOR; break;
            case '<': arg = OP_SHL; break;
            case '>': arg = OP_SHR; break;
            case '~': case '!': kind = KeyKind::UnOp; arg = UOP_NOT; break;
            case '=': kind = KeyKind::Equals; break;
            default: return false;
            }
        }
        break;
    }
    }

    press(kind, arg);
    flash(buttonFor(kind, arg));
    return true;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    // Typing works no matter which widget in the main window has focus;
    // menus and dialogs keep their own keyboard handling.
    if (ev->type() == QEvent::KeyPress && isActiveWindow()) {
        auto *w = qobject_cast<QWidget *>(obj);
        if (w && w->window() == this && handleKey(static_cast<QKeyEvent *>(ev)))
            return true;
    }
    return QMainWindow::eventFilter(obj, ev);
}

/* ------------------------------------------------------------------ */
/* History                                                             */

int MainWindow::entryIndexForRow(int row) const
{
    // Newest entries are shown first.
    return m_hist.items().size() - 1 - row;
}

void MainWindow::insertHistoryRow(int row, const HistoryEntry &e)
{
    auto *w = new QWidget;
    auto *box = new QVBoxLayout(w);
    box->setContentsMargins(12, 8, 12, 8);
    box->setSpacing(2);

    const QDateTime when = QDateTime::fromSecsSinceEpoch(e.time);
    auto *meta = makeLabel(QStringLiteral("%1 · %2-bit · %3")
                               .arg(QLatin1String(engine_base_name(Base(e.base))))
                               .arg(e.bits)
                               .arg(when.toString(QStringLiteral("MMM d, HH:mm"))),
                           "hMeta");
    auto *expr = makeLabel(softWrap(e.expr), "hExpr", Qt::AlignRight);
    auto *res = makeLabel(QStringLiteral("= ") + softWrap(e.result), "hResult", Qt::AlignRight);
    expr->setWordWrap(true);
    res->setWordWrap(true);
    box->addWidget(meta);
    box->addWidget(expr);
    box->addWidget(res);
    w->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *item = new QListWidgetItem;
    item->setToolTip(QStringLiteral("Click to use this result, right-click for more"));
    m_historyList->insertItem(row, item);
    m_historyList->setItemWidget(item, w);
    w->ensurePolished();
    const int width = HistoryWidth - 12 - 14;  // panel margins and scroll bar
    item->setSizeHint(QSize(width, w->heightForWidth(width)));
}

void MainWindow::rebuildHistory()
{
    m_historyList->clear();
    const auto &items = m_hist.items();
    for (int i = items.size() - 1; i >= 0; i--)
        insertHistoryRow(m_historyList->count(), items[i]);
    updateHistoryEmpty();
}

void MainWindow::updateHistoryEmpty()
{
    m_historyStack->setCurrentIndex(m_hist.items().isEmpty() ? 0 : 1);
}

void MainWindow::recordHistory(const EngineResult &r)
{
    HistoryEntry e;
    e.time = QDateTime::currentSecsSinceEpoch();
    e.bits = m_eng.bits;
    e.isSigned = m_eng.is_signed;
    e.base = int(m_eng.base);
    e.value = r.result;
    e.expr = QString::fromUtf8(r.expr);
    e.result = format(r.result, m_eng.base, m_set.groupDigits);

    int dropped = m_hist.add(e);
    insertHistoryRow(0, m_hist.items().last());
    while (dropped-- > 0 && m_historyList->count() > 0)
        delete m_historyList->takeItem(m_historyList->count() - 1);
    updateHistoryEmpty();
}

void MainWindow::useHistoryRow(int row)
{
    const int index = entryIndexForRow(row);
    if (index < 0 || index >= m_hist.items().size())
        return;
    engine_set_value(&m_eng, m_hist.items()[index].value);
    refresh();
    toast(QStringLiteral("Using %1").arg(format(m_eng.value, m_eng.base, m_set.groupDigits)));
}

void MainWindow::historyContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_historyList->itemAt(pos);
    if (!item)
        return;
    const int row = m_historyList->row(item);
    const int index = entryIndexForRow(row);
    if (index < 0 || index >= m_hist.items().size())
        return;
    const HistoryEntry e = m_hist.items()[index];

    QMenu menu(this);
    QAction *use = menu.addAction(QStringLiteral("Use result"));
    QAction *copyResult = menu.addAction(QStringLiteral("Copy result"));
    QAction *copyCalc = menu.addAction(QStringLiteral("Copy calculation"));
    menu.addSeparator();
    QAction *del = menu.addAction(QStringLiteral("Delete"));

    QAction *chosen = menu.exec(m_historyList->viewport()->mapToGlobal(pos));
    if (chosen == use) {
        useHistoryRow(row);
    } else if (chosen == copyResult) {
        copyText(e.result, QStringLiteral("result"));
    } else if (chosen == copyCalc) {
        copyText(QStringLiteral("%1 = %2").arg(e.expr, e.result), QStringLiteral("calculation"));
    } else if (chosen == del) {
        m_hist.removeAt(index);
        delete m_historyList->takeItem(row);
        updateHistoryEmpty();
    }
}

void MainWindow::showHistory(bool show, bool resizeWindow)
{
    if (m_historyWrap->isVisible() == show)
        return;
    const QSize before = size();
    m_historyWrap->setVisible(show);

    // Grow or shrink the window so the calculator keeps its width.
    if (resizeWindow && !isMaximized() && !isFullScreen()) {
        const int delta = HistoryWidth + HistoryGap;
        resize(qMax(before.width() + (show ? delta : -delta), minimumSizeHint().width()),
               before.height());
    }
}

/* ------------------------------------------------------------------ */
/* Clipboard and toast                                                 */

void MainWindow::copyText(const QString &text, const QString &what)
{
    QApplication::clipboard()->setText(text);
    toast(QStringLiteral("Copied %1").arg(what));
}

void MainWindow::copyValue()
{
    if (m_eng.error)
        return;
    const QString text = format(m_eng.value, m_eng.base, false);
    copyText(text, text);
}

void MainWindow::pasteValue()
{
    const QByteArray text = QApplication::clipboard()->text().toUtf8();
    uint64_t v = 0;

    switch (engine_parse(text.constData(), m_eng.base, m_eng.bits, m_eng.is_signed, &v)) {
    case PARSE_OK:
        engine_set_value(&m_eng, v);
        refresh();
        toast(QStringLiteral("Pasted %1").arg(format(v, m_eng.base, m_set.groupDigits)));
        break;
    case PARSE_RANGE:
        toast(QStringLiteral("Value doesn't fit in %1 bits").arg(m_eng.bits));
        break;
    case PARSE_EMPTY:
        toast(QStringLiteral("Clipboard is empty"));
        break;
    default:
        toast(QStringLiteral("Clipboard doesn't contain a %1 number")
                  .arg(QLatin1String(engine_base_name(m_eng.base))));
        break;
    }
}

void MainWindow::toast(const QString &message)
{
    m_toast->setText(message);
    m_toast->adjustSize();
    const QWidget *c = centralWidget();
    m_toast->move((c->width() - m_toast->width()) / 2, c->height() - m_toast->height() - 28);
    m_toast->show();
    m_toast->raise();

    m_toastAnim->stop();
    m_toastAnim->setStartValue(m_toastEffect->opacity());
    m_toastAnim->setEndValue(1.0);
    m_toastAnim->start();
    m_toastTimer->start(1800);
}

void MainWindow::resizeEvent(QResizeEvent *ev)
{
    QMainWindow::resizeEvent(ev);
    if (m_toast && m_toast->isVisible()) {
        const QWidget *c = centralWidget();
        m_toast->move((c->width() - m_toast->width()) / 2, c->height() - m_toast->height() - 28);
    }
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    m_set.geometry = saveGeometry();
    m_set.save();
    if (m_prefs)
        m_prefs->close();
    QMainWindow::closeEvent(ev);
}

/* ------------------------------------------------------------------ */
/* Dialogs                                                             */

void MainWindow::showPreferences()
{
    if (!m_prefs) {
        m_prefs = new PreferencesDialog(this);
        m_prefs->setAttribute(Qt::WA_DeleteOnClose);
    }
    m_prefs->show();
    m_prefs->raise();
    m_prefs->activateWindow();
}

void MainWindow::showShortcuts()
{
    struct Item { const char *keys; const char *what; };
    struct Group { const char *title; QVector<Item> items; };
    const QVector<Group> groups = {
        { "Numbers", { { "0 \xE2\x80\xA6 9", "Digits" }, { "A \xE2\x80\xA6 F", "Hex digits" },
                       { "Backspace", "Delete last digit" }, { "Delete", "Clear entry (CE)" },
                       { "Esc", "Clear all (AC)" }, { "F9", "Negate (\xC2\xB1)" } } },
        { "Arithmetic", { { "+", "Add" }, { "-", "Subtract" }, { "*", "Multiply" },
                          { "/", "Divide" }, { "%", "Remainder (MOD)" },
                          { "Enter  =", "Equals" } } },
        { "Bitwise", { { "&", "AND" }, { "|", "OR" }, { "^", "XOR" }, { "~  !", "NOT" },
                       { "<", "Shift left" }, { ">", "Shift right" } } },
        { "Base and word size", { { "F5", "Hexadecimal" }, { "F6", "Decimal" }, { "F7", "Octal" },
                                  { "F8", "Binary" }, { "F12", "64-bit (QWORD)" },
                                  { "F2", "32-bit (DWORD)" }, { "F3", "16-bit (WORD)" },
                                  { "F4", "8-bit (BYTE)" }, { "Ctrl+S", "Toggle signed" } } },
        { "Application", { { "Ctrl+C", "Copy value" }, { "Ctrl+V", "Paste value" },
                           { "Ctrl+H", "History" }, { "Ctrl+B", "Bit grid" },
                           { "Ctrl+G", "Group digits" }, { "Ctrl+,", "Preferences" },
                           { "F1", "Keyboard shortcuts" }, { "Ctrl+Q", "Quit" } } },
    };

    auto *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(QStringLiteral("Keyboard Shortcuts"));

    auto *grid = new QGridLayout(dlg);
    grid->setContentsMargins(24, 20, 24, 20);
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(18);

    const int columns = 2;
    for (int g = 0; g < groups.size(); g++) {
        auto *section = new QWidget;
        auto *form = new QGridLayout(section);
        form->setContentsMargins(0, 0, 0, 0);
        form->setHorizontalSpacing(12);
        form->setVerticalSpacing(6);
        form->addWidget(makeLabel(QLatin1String(groups[g].title), "sectionLabel"), 0, 0, 1, 2);
        int row = 1;
        for (const Item &it : groups[g].items) {
            auto *keys = new QWidget;
            auto *kl = new QHBoxLayout(keys);
            kl->setContentsMargins(0, 0, 0, 0);
            kl->setSpacing(4);
            for (const QString &k : u8(it.keys).split(QStringLiteral("  ")))
                kl->addWidget(makeLabel(k, "keycap", Qt::AlignCenter));
            kl->addStretch(1);
            form->addWidget(keys, row, 0);
            form->addWidget(makeLabel(u8(it.what), "dimLabel"), row, 1);
            row++;
        }
        form->setRowStretch(row, 1);
        form->setColumnStretch(1, 1);
        grid->addWidget(section, g / columns, g % columns, Qt::AlignTop);
    }
    dlg->show();
}

void MainWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("About Hex Calc"));
    box.setIconPixmap(windowIcon().pixmap(72, 72));
    box.setTextFormat(Qt::RichText);
    box.setText(QStringLiteral(
        "<h3>Hex Calc %1</h3>"
        "<p>A themeable programmer's calculator for Linux.<br>"
        "Hex, decimal, octal and binary with bitwise operations.</p>"
        "<p><a href=\"https://github.com/naved124/hex-calc\">github.com/naved124/hex-calc</a></p>"
        "<p>Released under the MIT license. Built with Qt %2.</p>")
                    .arg(QStringLiteral(HEXCALC_VERSION), QLatin1String(qVersion())));
    box.exec();
}
