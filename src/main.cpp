/*
 * Hex Calc - a themeable programmer's calculator for Linux.
 */
#include "mainwindow.h"
#include "themes.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QProcess>
#include <QStyleFactory>
#include <QStyleHints>

#include <cstdio>

static bool isDataDir(const QString &dir)
{
    return !dir.isEmpty() && QFileInfo(dir + QStringLiteral("/fonts")).isDir();
}

// Fonts and icons live in $HEXCALC_DATADIR, <exe>/../share/hex-calc
// (installed or unpacked release), <exe>/assets or <exe>/../assets (source
// tree builds), or the compiled-in install location.
static QString findDataDir()
{
    const QString env = qEnvironmentVariable("HEXCALC_DATADIR");
    if (isDataDir(env))
        return env;

    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString candidates[] = {
        exeDir + QStringLiteral("/../share/hex-calc"),
        exeDir + QStringLiteral("/assets"),
        exeDir + QStringLiteral("/../assets"),
        QStringLiteral(HEXCALC_DATADIR),
    };
    for (const QString &c : candidates)
        if (isDataDir(c))
            return QDir(c).canonicalPath();
    return QString();
}

static int registerFonts(const QString &dataDir)
{
    int count = 0;
    if (dataDir.isEmpty())
        return 0;
    const QDir dir(dataDir + QStringLiteral("/fonts"));
    const QStringList files = dir.entryList({ QStringLiteral("*.ttf"), QStringLiteral("*.otf") },
                                            QDir::Files);
    for (const QString &f : files)
        if (QFontDatabase::addApplicationFont(dir.filePath(f)) >= 0)
            count++;
    return count;
}

// Best effort: Qt's colour scheme hint, then GNOME's setting, then the
// lightness of the palette the platform gave us.
static bool detectSystemDark()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme != Qt::ColorScheme::Unknown)
        return scheme == Qt::ColorScheme::Dark;
#endif
    QProcess gs;
    gs.start(QStringLiteral("gsettings"),
             { QStringLiteral("get"), QStringLiteral("org.gnome.desktop.interface"),
               QStringLiteral("color-scheme") });
    if (gs.waitForFinished(800) && gs.exitCode() == 0) {
        const QByteArray out = gs.readAllStandardOutput();
        if (out.contains("prefer-dark"))
            return true;
        if (out.contains("prefer-light"))
            return false;
    }
    if (qEnvironmentVariable("GTK_THEME").contains(QStringLiteral("dark"), Qt::CaseInsensitive))
        return true;
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

struct Options {
    QCommandLineOption theme{ { QStringLiteral("t"), QStringLiteral("theme") },
                              QStringLiteral("Start with this colour theme (see --list-themes)."),
                              QStringLiteral("id") };
    QCommandLineOption listThemes{ QStringLiteral("list-themes"),
                                   QStringLiteral("List the available themes and exit.") };
};

static void setupParser(QCommandLineParser &parser, Options &opts)
{
    parser.setApplicationDescription(QStringLiteral("A themeable programmer's calculator."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(opts.theme);
    parser.addOption(opts.listThemes);
}

// --help, --version and --list-themes must work without a display.
static bool isInfoRequest(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        const QByteArray a(argv[i]);
        if (a == "-h" || a == "--help" || a == "--help-all" || a == "-v" || a == "--version" ||
            a == "--list-themes")
            return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral("hex-calc"));
    QCoreApplication::setApplicationVersion(QStringLiteral(HEXCALC_VERSION));

    if (isInfoRequest(argc, argv)) {
        QCoreApplication core(argc, argv);
        QCommandLineParser parser;
        Options opts;
        setupParser(parser, opts);
        parser.process(core);  // exits for --help / --version
        if (parser.isSet(opts.listThemes)) {
            std::printf("%-18s %s\n", THEME_FOLLOW_SYSTEM, "Follow system (Material light/dark)");
            for (int i = 0; i < Themes::count(); i++)
                std::printf("%-18s %s\n", Themes::at(i).id, Themes::at(i).name);
        }
        return 0;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication::setApplicationDisplayName(QStringLiteral("Hex Calc"));
    QApplication::setDesktopFileName(QStringLiteral(HEXCALC_APP_ID));
    QApplication app(argc, argv);

    QCommandLineParser parser;
    Options opts;
    setupParser(parser, opts);
    parser.process(app);

    const bool systemDark = detectSystemDark();
    // Fusion draws consistently everywhere and fully honours style sheets.
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    const QString dataDir = findDataDir();
    if (registerFonts(dataDir) == 0)
        qInfo("Bundled fonts not found; using system fonts only");

    QIcon icon = QIcon::fromTheme(QStringLiteral("hex-calc"));
    if (icon.isNull() && !dataDir.isEmpty())
        icon = QIcon(dataDir + QStringLiteral("/hex-calc.png"));
    QApplication::setWindowIcon(icon);

    MainWindow window(dataDir, systemDark, parser.value(opts.theme));
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, &window,
                     [&window](Qt::ColorScheme s) { window.setSystemDark(s == Qt::ColorScheme::Dark); });
#endif
    window.show();
    return app.exec();
}
