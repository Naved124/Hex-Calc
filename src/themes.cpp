/*
 * Hex Calc - colour themes, Qt palettes and style sheets.
 */
#include "themes.h"

#include <QMap>
#include <QtGlobal>

/*      id                  name                dark
 *      bg         surface    surfaceHi  fg         fgDim
 *      primary    onPrimary  secondary  onSecond   tertiary   onTert
 *      error      onError    danger     outline */
static const Theme THEMES[] = {
    { "material-dark", "Material Dark", true,
      "#141218", "#1D1B20", "#2B2930", "#E6E0E9", "#CAC4D0",
      "#D0BCFF", "#381E72", "#4A4458", "#E8DEF8", "#633B48", "#FFD8E4",
      "#8C1D18", "#F9DEDC", "#F2B8B5", "#49454F" },
    { "material-light", "Material Light", false,
      "#FEF7FF", "#F3EDF7", "#E6E0E9", "#1D1B20", "#49454F",
      "#6750A4", "#FFFFFF", "#E8DEF8", "#1D192B", "#FFD8E4", "#31111D",
      "#F9DEDC", "#410E0B", "#B3261E", "#CAC4D0" },
    { "monokai", "Monokai", true,
      "#272822", "#31322B", "#3E3D32", "#F8F8F2", "#A59F85",
      "#A6E22E", "#272822", "#383830", "#66D9EF", "#383830", "#AE81FF",
      "#F92672", "#272822", "#F92672", "#49483E" },
    { "dracula", "Dracula", true,
      "#282A36", "#21222C", "#343746", "#F8F8F2", "#A3AACC",
      "#BD93F9", "#282A36", "#44475A", "#8BE9FD", "#44475A", "#FF79C6",
      "#FF5555", "#282A36", "#FF6E6E", "#44475A" },
    { "nord", "Nord", true,
      "#2E3440", "#3B4252", "#434C5E", "#ECEFF4", "#B6BECC",
      "#88C0D0", "#2E3440", "#4C566A", "#8FBCBB", "#4C566A", "#B48EAD",
      "#BF616A", "#ECEFF4", "#E0838B", "#4C566A" },
    { "solarized-dark", "Solarized Dark", true,
      "#002B36", "#073642", "#0E4250", "#EEE8D5", "#93A1A1",
      "#268BD2", "#FDF6E3", "#0E4250", "#2AA198", "#0E4250", "#D33682",
      "#DC322F", "#FDF6E3", "#E8605D", "#1E4B57" },
    { "solarized-light", "Solarized Light", false,
      "#FDF6E3", "#EEE8D5", "#E4DCC6", "#073642", "#586E75",
      "#268BD2", "#FDF6E3", "#D7E5E0", "#0F6E68", "#EBD9DE", "#A3245F",
      "#DC322F", "#FDF6E3", "#C52A27", "#D6CFBA" },
    { "gruvbox-dark", "Gruvbox Dark", true,
      "#282828", "#32302F", "#3C3836", "#EBDBB2", "#A89984",
      "#FABD2F", "#282828", "#3C3836", "#83A598", "#3C3836", "#D3869B",
      "#FB4934", "#282828", "#FB4934", "#504945" },
    { "one-dark", "One Dark", true,
      "#282C34", "#21252B", "#2F343E", "#DCDFE4", "#9DA5B4",
      "#61AFEF", "#282C34", "#2F343E", "#56B6C2", "#2F343E", "#C678DD",
      "#E06C75", "#282C34", "#E06C75", "#3E4451" },
    { "catppuccin-mocha", "Catppuccin Mocha", true,
      "#1E1E2E", "#181825", "#313244", "#CDD6F4", "#A6ADC8",
      "#CBA6F7", "#1E1E2E", "#313244", "#89B4FA", "#313244", "#F5C2E7",
      "#F38BA8", "#1E1E2E", "#F38BA8", "#45475A" },
    { "catppuccin-latte", "Catppuccin Latte", false,
      "#EFF1F5", "#E6E9EF", "#DCE0E8", "#4C4F69", "#6C6F85",
      "#8839EF", "#EFF1F5", "#D5DBF0", "#1E66F5", "#D6EBEA", "#137F86",
      "#D20F39", "#EFF1F5", "#D20F39", "#CCD0DA" },
    { "tokyo-night", "Tokyo Night", true,
      "#1A1B26", "#16161E", "#24283B", "#C0CAF5", "#9AA5CE",
      "#7AA2F7", "#1A1B26", "#292E42", "#7DCFFF", "#292E42", "#BB9AF7",
      "#F7768E", "#1A1B26", "#F7768E", "#3B4261" },
};

namespace Themes {

int count()
{
    return int(sizeof THEMES / sizeof THEMES[0]);
}

const Theme &at(int index)
{
    return THEMES[qBound(0, index, count() - 1)];
}

const Theme *find(const QString &id)
{
    for (const Theme &t : THEMES)
        if (id == QLatin1String(t.id))
            return &t;
    return nullptr;
}

const Theme &fallback(bool dark)
{
    return dark ? THEMES[0] : THEMES[1];
}

static QColor mix(const char *a, const char *b, double t)
{
    const QColor ca{QLatin1String(a)}, cb{QLatin1String(b)};
    return QColor::fromRgbF(ca.redF() + (cb.redF() - ca.redF()) * t,
                            ca.greenF() + (cb.greenF() - ca.greenF()) * t,
                            ca.blueF() + (cb.blueF() - ca.blueF()) * t);
}

static QString hex(const QColor &c)
{
    return c.name(QColor::HexRgb);
}

static QString rgba(const char *color, double alpha)
{
    const QColor c{QLatin1String(color)};
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(qRound(alpha * 255));
}

QPalette palette(const Theme &t)
{
    auto c = [](const char *s) { return QColor(QLatin1String(s)); };
    QPalette p;
    QColor disabled = mix(t.fg, t.bg, 0.55);

    p.setColor(QPalette::Window, c(t.bg));
    p.setColor(QPalette::WindowText, c(t.fg));
    p.setColor(QPalette::Base, c(t.surface));
    p.setColor(QPalette::AlternateBase, c(t.surfaceHi));
    p.setColor(QPalette::Text, c(t.fg));
    p.setColor(QPalette::Button, c(t.surfaceHi));
    p.setColor(QPalette::ButtonText, c(t.fg));
    p.setColor(QPalette::BrightText, c(t.danger));
    p.setColor(QPalette::Highlight, c(t.primary));
    p.setColor(QPalette::HighlightedText, c(t.onPrimary));
    p.setColor(QPalette::ToolTipBase, c(t.surfaceHi));
    p.setColor(QPalette::ToolTipText, c(t.fg));
    p.setColor(QPalette::Link, c(t.primary));
    p.setColor(QPalette::LinkVisited, c(t.primary));
    p.setColor(QPalette::Light, c(t.surfaceHi));
    p.setColor(QPalette::Midlight, c(t.surfaceHi));
    p.setColor(QPalette::Mid, c(t.outline));
    p.setColor(QPalette::Dark, c(t.outline));
    p.setColor(QPalette::Shadow, QColor(0, 0, 0, 120));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    p.setColor(QPalette::PlaceholderText, c(t.fgDim));
#endif
    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text, disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Highlight, c(t.outline));
    return p;
}

/* Values in {braces} are filled in by styleSheet(). Every rule that the
 * calculator relies on is explicit, so the result does not depend on the
 * desktop's Qt style. */
static const char STYLE_TEMPLATE[] = R"QSS(
QWidget { color: {fg}; {ui_family} font-size: {u}pt; }
QMainWindow, QDialog, QMessageBox, #central { background-color: {bg}; }
QLabel { background: transparent; }
QToolTip {
  background-color: {surface_hi}; color: {fg}; border: 1px solid {outline};
  padding: 4px 8px; border-radius: 6px;
}

#appTitle { font-weight: bold; font-size: {u1}pt; }
QFrame#card { background-color: {surface}; border-radius: 20px; }
#modeLabel { color: {fg_dim}; font-size: {small}pt; }
#memIndicator {
  background-color: {primary_a18}; color: {primary}; border-radius: 8px;
  padding: 1px 8px; font-weight: bold; font-size: {small}pt;
}
#exprLabel { color: {fg_dim}; {mono_family} font-size: {expr}pt; }
#valueLabel { color: {fg}; {mono_family} font-size: {d}pt; }
#valueLabel[len="l"] { font-size: {d_l}pt; }
#valueLabel[len="xl"] { font-size: {d_xl}pt; }
#valueLabel[len="xxl"] { font-size: {d_xxl}pt; }
#valueLabel[error="true"] { color: {danger}; font-size: {d_err}pt; }

QPushButton {
  background-color: {surface_hi}; color: {fg}; border: none;
  border-radius: 12px; padding: 6px 14px;
}
QPushButton:hover { background-color: {surface_hi_hover}; }
QPushButton:pressed { background-color: {surface_hi_press}; }
QPushButton:disabled { color: {fg_a38}; }

QToolButton {
  background: transparent; border: none; border-radius: 8px; padding: 4px;
}
QToolButton:hover { background-color: {fg_a09}; }
QToolButton:pressed { background-color: {fg_a16}; }
QToolButton:checked { background-color: {primary_a22}; }
QToolButton::menu-indicator { image: none; width: 0px; }

QPushButton[kind="mem"] {
  background: transparent; color: {primary}; font-weight: bold;
  padding: 2px 8px; border-radius: 8px; font-size: {small}pt;
}
QPushButton[kind="mem"]:hover { background-color: {primary_a14}; }
QPushButton[kind="mem"]:pressed { background-color: {primary_a26}; }
QPushButton[kind="mem"]:disabled { color: {fg_dim_a45}; background: transparent; }

QFrame#baseRow { background: transparent; border-radius: 14px; }
QFrame#baseRow:hover { background-color: {fg_a05}; }
QFrame#baseRow[active="true"] { background-color: {primary_a16}; }
#baseName { color: {fg_dim}; font-weight: bold; min-width: 3.2em; }
#baseName[active="true"] { color: {primary}; }
#baseValue { {mono_family} font-size: {base}pt; }
#baseValue[long="true"] { font-size: {base_long}pt; }

QPushButton[kind="bit"] {
  background: transparent; color: {fg_dim_a75}; padding: 0px;
  min-width: {bit_w}px; max-width: {bit_w}px; min-height: 20px; border-radius: 4px;
  {mono_family} font-size: {bit}pt;
}
QPushButton[kind="bit"][on="true"] { color: {primary}; font-weight: bold; }
QPushButton[kind="bit"]:hover { background-color: {primary_a16}; }
QPushButton[kind="bit"]:disabled { color: {fg_dim_a22}; background: transparent; }
#bitIndex { color: {fg_dim_a80}; {mono_family} font-size: {bit_index}pt; }

QPushButton[kind="chip"] {
  background-color: {surface}; color: {fg_dim}; border: 1px solid {outline};
  border-radius: 0px; padding: 4px 12px; font-weight: bold;
}
QPushButton[kind="chip"]:hover { background-color: {surface_hover}; }
QPushButton[kind="chip"]:checked { background-color: {secondary}; color: {on_secondary}; }
QPushButton[kind="chip"][seg="first"] {
  border-top-left-radius: 11px; border-bottom-left-radius: 11px;
}
QPushButton[kind="chip"][seg="mid"] { border-left: none; }
QPushButton[kind="chip"][seg="last"] {
  border-left: none; border-top-right-radius: 11px; border-bottom-right-radius: 11px;
}

QPushButton[kind="digit"], QPushButton[kind="op"], QPushButton[kind="bitop"],
QPushButton[kind="clear"], QPushButton[kind="func"], QPushButton[kind="equals"] {
  border-radius: 16px; padding: 0px; min-height: 42px; font-size: {key}pt; font-weight: bold;
}
QPushButton[kind="digit"] {
  background-color: {surface_hi}; color: {fg}; {mono_family} font-size: {digit}pt; font-weight: normal;
}
QPushButton[kind="digit"]:hover { background-color: {surface_hi_hover}; }
QPushButton[kind="digit"]:pressed, QPushButton[kind="digit"][flash="true"] { background-color: {surface_hi_press}; }
QPushButton[kind="digit"]:disabled { background-color: {surface_hi_a40}; color: {fg_a25}; }
QPushButton[kind="op"] { background-color: {secondary}; color: {on_secondary}; }
QPushButton[kind="op"]:hover { background-color: {secondary_hover}; }
QPushButton[kind="op"]:pressed, QPushButton[kind="op"][flash="true"] { background-color: {secondary_press}; }
QPushButton[kind="bitop"] { background-color: {tertiary}; color: {on_tertiary}; }
QPushButton[kind="bitop"]:hover { background-color: {tertiary_hover}; }
QPushButton[kind="bitop"]:pressed, QPushButton[kind="bitop"][flash="true"] { background-color: {tertiary_press}; }
QPushButton[kind="clear"] { background-color: {error}; color: {on_error}; }
QPushButton[kind="clear"]:hover { background-color: {error_hover}; }
QPushButton[kind="clear"]:pressed, QPushButton[kind="clear"][flash="true"] { background-color: {error_press}; }
QPushButton[kind="func"] { background-color: {surface_hi}; color: {primary}; }
QPushButton[kind="func"]:hover { background-color: {func_hover}; }
QPushButton[kind="func"]:pressed, QPushButton[kind="func"][flash="true"] { background-color: {func_press}; }
QPushButton[kind="equals"] { background-color: {primary}; color: {on_primary}; }
QPushButton[kind="equals"]:hover { background-color: {primary_hover}; }
QPushButton[kind="equals"]:pressed, QPushButton[kind="equals"][flash="true"] { background-color: {primary_press}; }
QPushButton[sym="true"] { font-size: {sym}pt; }

QFrame#historyPanel { background-color: {surface}; border-radius: 20px; }
#historyTitle { font-weight: bold; font-size: {u1}pt; }
QListWidget { background: transparent; border: none; outline: 0; }
QListWidget::item { border-radius: 12px; }
QListWidget::item:hover { background-color: {fg_a06}; }
QListWidget::item:selected { background-color: {primary_a12}; }
#hMeta { color: {fg_dim}; font-size: {small}pt; }
#hExpr { color: {fg_dim}; {mono_family} font-size: {hexpr}pt; }
#hResult { color: {fg}; font-weight: bold; {mono_family} font-size: {hres}pt; }
#emptyLabel { color: {fg_dim}; }
#toast {
  background-color: {fg}; color: {bg}; border-radius: 12px; padding: 10px 18px;
}

QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background-color: {fg_a30}; border-radius: 3px; min-height: 28px; }
QScrollBar::handle:vertical:hover { background-color: {fg_a50}; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background-color: {fg_a30}; border-radius: 3px; min-width: 28px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; }
QScrollBar::add-page, QScrollBar::sub-page { background: none; }

QMenu {
  background-color: {surface}; color: {fg}; border: 1px solid {outline}; padding: 6px;
}
QMenu::item { padding: 7px 28px 7px 28px; border-radius: 6px; background: transparent; }
QMenu::item:selected { background-color: {primary_a20}; }
QMenu::item:disabled { color: {fg_a38}; }
QMenu::separator { height: 1px; background: {outline}; margin: 5px 8px; }
QMenu::right-arrow { width: 8px; height: 8px; }

QComboBox, QSpinBox, QLineEdit {
  background-color: {surface}; color: {fg}; border: 1px solid {outline};
  border-radius: 10px; padding: 5px 10px; min-height: 22px;
}
QComboBox:hover, QSpinBox:hover, QLineEdit:hover { border-color: {fg_dim}; }
QComboBox:focus, QSpinBox:focus, QLineEdit:focus { border-color: {primary}; }
QComboBox::drop-down { border: none; width: 26px; }
QComboBox::down-arrow { image: url({arrow_down}); width: 10px; height: 10px; }
QSpinBox { padding-right: 26px; }
QSpinBox::up-button, QSpinBox::down-button {
  subcontrol-origin: border; width: 24px; border: none; background: transparent;
}
QSpinBox::up-button { subcontrol-position: top right; border-top-right-radius: 10px; }
QSpinBox::down-button { subcontrol-position: bottom right; border-bottom-right-radius: 10px; }
QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: {fg_a09}; }
QSpinBox::up-arrow { image: url({arrow_up}); width: 10px; height: 10px; }
QSpinBox::down-arrow { image: url({arrow_down}); width: 10px; height: 10px; }
QComboBox QAbstractItemView {
  background-color: {surface}; color: {fg}; border: 1px solid {outline};
  selection-background-color: {primary_a22}; selection-color: {fg}; outline: 0;
}
QCheckBox { spacing: 8px; }

#sectionLabel { font-weight: bold; font-size: {u1}pt; }
#dimLabel { color: {fg_dim}; }
#keycap {
  background-color: {surface_hi}; color: {fg}; border: 1px solid {outline};
  border-bottom: 3px solid {outline}; border-radius: 6px; padding: 1px 8px;
  {mono_family} font-size: {small}pt;
}
#preview {
  background-color: {surface}; border-radius: 14px; padding: 12px 16px;
  {mono_family} font-size: {preview}pt;
}
)QSS";

static QString familyDecl(const QString &family)
{
    if (family.isEmpty())
        return QString();
    QString clean = family;
    clean.remove(QLatin1Char('"')).remove(QLatin1Char('\\')).remove(QLatin1Char(';'))
        .remove(QLatin1Char('{')).remove(QLatin1Char('}'));
    return QStringLiteral("font-family: \"%1\";").arg(clean);
}

QString styleSheet(const Theme &t, const FontChoice &f, const StyleAssets &assets)
{
    const int d = qBound(12, f.monoSize, 72);
    const int u = qBound(8, f.uiSize, 24);
    QMap<QString, QString> v;

    v[QStringLiteral("bg")] = QLatin1String(t.bg);
    v[QStringLiteral("surface")] = QLatin1String(t.surface);
    v[QStringLiteral("surface_hi")] = QLatin1String(t.surfaceHi);
    v[QStringLiteral("fg")] = QLatin1String(t.fg);
    v[QStringLiteral("fg_dim")] = QLatin1String(t.fgDim);
    v[QStringLiteral("primary")] = QLatin1String(t.primary);
    v[QStringLiteral("on_primary")] = QLatin1String(t.onPrimary);
    v[QStringLiteral("secondary")] = QLatin1String(t.secondary);
    v[QStringLiteral("on_secondary")] = QLatin1String(t.onSecondary);
    v[QStringLiteral("tertiary")] = QLatin1String(t.tertiary);
    v[QStringLiteral("on_tertiary")] = QLatin1String(t.onTertiary);
    v[QStringLiteral("error")] = QLatin1String(t.error);
    v[QStringLiteral("on_error")] = QLatin1String(t.onError);
    v[QStringLiteral("danger")] = QLatin1String(t.danger);
    v[QStringLiteral("outline")] = QLatin1String(t.outline);

    v[QStringLiteral("surface_hover")] = hex(mix(t.surface, t.fg, 0.07));
    v[QStringLiteral("surface_hi_hover")] = hex(mix(t.surfaceHi, t.fg, 0.08));
    v[QStringLiteral("surface_hi_press")] = hex(mix(t.surfaceHi, t.fg, 0.20));
    v[QStringLiteral("secondary_hover")] = hex(mix(t.secondary, t.onSecondary, 0.08));
    v[QStringLiteral("secondary_press")] = hex(mix(t.secondary, t.onSecondary, 0.20));
    v[QStringLiteral("tertiary_hover")] = hex(mix(t.tertiary, t.onTertiary, 0.08));
    v[QStringLiteral("tertiary_press")] = hex(mix(t.tertiary, t.onTertiary, 0.20));
    v[QStringLiteral("error_hover")] = hex(mix(t.error, t.onError, 0.10));
    v[QStringLiteral("error_press")] = hex(mix(t.error, t.onError, 0.22));
    v[QStringLiteral("func_hover")] = hex(mix(t.surfaceHi, t.primary, 0.10));
    v[QStringLiteral("func_press")] = hex(mix(t.surfaceHi, t.primary, 0.22));
    v[QStringLiteral("primary_hover")] = hex(mix(t.primary, t.onPrimary, 0.10));
    v[QStringLiteral("primary_press")] = hex(mix(t.primary, t.onPrimary, 0.24));

    v[QStringLiteral("primary_a12")] = rgba(t.primary, 0.12);
    v[QStringLiteral("primary_a14")] = rgba(t.primary, 0.14);
    v[QStringLiteral("primary_a16")] = rgba(t.primary, 0.16);
    v[QStringLiteral("primary_a18")] = rgba(t.primary, 0.18);
    v[QStringLiteral("primary_a20")] = rgba(t.primary, 0.20);
    v[QStringLiteral("primary_a22")] = rgba(t.primary, 0.22);
    v[QStringLiteral("primary_a26")] = rgba(t.primary, 0.26);
    v[QStringLiteral("fg_a05")] = rgba(t.fg, 0.05);
    v[QStringLiteral("fg_a06")] = rgba(t.fg, 0.06);
    v[QStringLiteral("fg_a09")] = rgba(t.fg, 0.09);
    v[QStringLiteral("fg_a16")] = rgba(t.fg, 0.16);
    v[QStringLiteral("fg_a25")] = rgba(t.fg, 0.25);
    v[QStringLiteral("fg_a30")] = rgba(t.fg, 0.30);
    v[QStringLiteral("fg_a38")] = rgba(t.fg, 0.38);
    v[QStringLiteral("fg_a50")] = rgba(t.fg, 0.50);
    v[QStringLiteral("fg_dim_a22")] = rgba(t.fgDim, 0.22);
    v[QStringLiteral("fg_dim_a45")] = rgba(t.fgDim, 0.45);
    v[QStringLiteral("fg_dim_a75")] = rgba(t.fgDim, 0.75);
    v[QStringLiteral("fg_dim_a80")] = rgba(t.fgDim, 0.80);
    v[QStringLiteral("surface_hi_a40")] = rgba(t.surfaceHi, 0.40);

    v[QStringLiteral("arrow_down")] = QStringLiteral("%1/down-%2.png").arg(assets.arrowDir, assets.arrowTag);
    v[QStringLiteral("arrow_up")] = QStringLiteral("%1/up-%2.png").arg(assets.arrowDir, assets.arrowTag);
    v[QStringLiteral("mono_family")] = familyDecl(f.monoFamily);
    v[QStringLiteral("ui_family")] = familyDecl(f.uiFamily);
    v[QStringLiteral("u")] = QString::number(u);
    v[QStringLiteral("u1")] = QString::number(u + 1);
    v[QStringLiteral("small")] = QString::number(qMax(u - 1, 7));
    v[QStringLiteral("d")] = QString::number(d);
    v[QStringLiteral("d_l")] = QString::number(d * 4 / 5);
    v[QStringLiteral("d_xl")] = QString::number(d * 5 / 8);
    v[QStringLiteral("d_xxl")] = QString::number(qMax(d * 7 / 16, u + 1));
    v[QStringLiteral("d_err")] = QString::number(qMax(d / 2, u + 2));
    v[QStringLiteral("expr")] = QString::number(qMax(u + 1, d * 2 / 5));
    v[QStringLiteral("base")] = QString::number(u + 1);
    v[QStringLiteral("base_long")] = QString::number(qMax(u, 8));
    v[QStringLiteral("key")] = QString::number(u + 1);
    v[QStringLiteral("digit")] = QString::number(u + 4);
    v[QStringLiteral("sym")] = QString::number(u + 7);
    v[QStringLiteral("bit")] = QString::number(qMax(u - 1, 7));
    v[QStringLiteral("bit_w")] = QString::number(qMax(u - 1, 7) + 2);
    v[QStringLiteral("bit_index")] = QString::number(qMax(u - 3, 6));
    v[QStringLiteral("hres")] = QString::number(u + 3);
    v[QStringLiteral("hexpr")] = QString::number(qMax(u - 1, 7));
    v[QStringLiteral("preview")] = QString::number(qMax(d * 2 / 3, 12));

    QString css = QString::fromUtf8(STYLE_TEMPLATE);
    for (auto it = v.constBegin(); it != v.constEnd(); ++it)
        css.replace(QLatin1Char('{') + it.key() + QLatin1Char('}'), it.value());
    return css;
}

} // namespace Themes
