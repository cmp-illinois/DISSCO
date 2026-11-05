// Save -> reopen round-trip test for LASSIE projects. -- Tony Chai, 2026
//
// For each project file, this drives LASSIE's real main window offscreen,
// like a user would: open it, save it, reopen the saved file, and save again.
// It then checks the *results*, not how LASSIE produced them:
//   - the project opens and saves without errors or pop-up dialogs,
//   - nothing in the original file is lost or changed by saving,
//   - saving an unchanged project a second time gives an identical file.
// The round trip happens in a folder whose name has spaces, an accent and
// Chinese characters, as a user's folder or Windows user name might.
// It also opens each project from a read-only folder and checks that all of
// its envelopes are loaded, and repeats the round trip from a copy with
// Windows (CRLF) line endings, as LASSIE saves projects on Windows.
//
// Run through ctest (LASSIE.ProjectSaveRoundTrip). The project files used are
// the tutorial examples in Documents/examples/ plus
// LASSIE/tests/data/roundtrip_all_objects.dissco, which holds one of every
// kind of object a project can contain.

#include "ProjectRoundTripTest.hpp"

#include "../src/core/EnvelopeLibraryEntry.hpp"
#include "../src/core/project_struct.hpp"
#include "../src/inst.hpp"
#include "../src/windows/MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QProcess>
#include <QMessageBox>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>

#include <iostream>
#include <vector>

namespace {

std::string currentTest;
int currentTestFailures = 0;
// Set by a test that couldn't run here, so it isn't reported as OK.
bool currentTestSkipped = false;

void fail(const QString& message)
{
    std::cerr << "FAILED [" << currentTest << "]: "
              << message.toStdString() << '\n';
    ++currentTestFailures;
}

// Thrown to stop just the current test when the following checks can't run
// (e.g. the project didn't open, so there's nothing to save).
struct RequirementFailed {
    QString message;
};

void require(bool condition, const QString& message)
{
    if (!condition)
        throw RequirementFailed{message};
}

// Any dialog that pops up during the test would wait forever for a click
// that never comes. Record it as a failure and close it instead.
class DialogWatchdog {
public:
    DialogWatchdog()
    {
        timer_.setInterval(50);
        QObject::connect(&timer_, &QTimer::timeout, [] {
            QWidget* modal = QApplication::activeModalWidget();
            if (!modal)
                return;
            // LASSIE shows Project Properties whenever a project opens; click
            // OK like a user would (the checks afterwards catch any change).
            if (modal->windowTitle() == QStringLiteral("Project Properties")) {
                if (auto* dialog = qobject_cast<QDialog*>(modal)) {
                    dialog->accept();
                    return;
                }
            }
            QString text = modal->windowTitle();
            if (auto* box = qobject_cast<QMessageBox*>(modal))
                text += QStringLiteral(": ") + box->text();
            fail(QStringLiteral("unexpected dialog appeared (\"%1\")")
                     .arg(text.simplified()));
            if (auto* dialog = qobject_cast<QDialog*>(modal))
                dialog->reject();
            else
                modal->close();
        });
        timer_.start();
    }

private:
    QTimer timer_;
};

// Flattens an XML file into "path = value" lines, so two files can be
// compared value by value regardless of indentation or line breaks.
// <Event> elements are identified by their <Name> rather than by position,
// so reordering events in the file doesn't count as a change.
void flatten(const QDomElement& element, const QString& path, QStringList& out)
{
    const QDomNamedNodeMap attributes = element.attributes();
    for (int i = 0; i < attributes.count(); ++i) {
        const QDomAttr attribute = attributes.item(i).toAttr();
        out << path + QStringLiteral("/@") + attribute.name()
                   + QStringLiteral(" = ") + attribute.value().simplified();
    }

    QString text;
    QHash<QString, int> seenTags;
    bool hasChildElements = false;
    for (QDomNode child = element.firstChild(); !child.isNull();
         child = child.nextSibling()) {
        if (child.isText() || child.isCDATASection()) {
            text += child.nodeValue();
        } else if (child.isElement()) {
            hasChildElements = true;
            const QDomElement childElement = child.toElement();
            const QString tag = childElement.tagName();
            QString key;
            if (tag == QStringLiteral("Event")) {
                key = QStringLiteral("Event[%1]").arg(
                    childElement.firstChildElement(QStringLiteral("Name")).text());
            } else {
                key = QStringLiteral("%1[%2]").arg(tag).arg(seenTags[tag]++);
            }
            flatten(childElement, path + QLatin1Char('/') + key, out);
        }
    }
    if (!hasChildElements) {
        out << path + QStringLiteral(" = ") + text.simplified();
    } else if (!text.trimmed().isEmpty()) {
        // Text mixed with <Fun> elements is a formula such as "2 * <Fun> + 1".
        // LASSIE trims the spaces next to each <Fun>, which doesn't change the
        // math, so only the characters are compared here.
        text.remove(QRegularExpression(QStringLiteral("\\s")));
        out << path + QStringLiteral(" = ") + text;
    }
}

QStringList flattenFile(const QString& filePath)
{
    QFile file(filePath);
    require(file.open(QIODevice::ReadOnly),
            QStringLiteral("could not read %1").arg(filePath));
    QDomDocument document;
    const auto parsed = document.setContent(&file);
    require(static_cast<bool>(parsed),
            QStringLiteral("%1 is not valid XML: %2")
                .arg(filePath, parsed.errorMessage));
    QStringList lines;
    flatten(document.documentElement(), document.documentElement().tagName(), lines);
    return lines;
}

QByteArray readAll(const QString& filePath)
{
    QFile file(filePath);
    require(file.open(QIODevice::ReadOnly),
            QStringLiteral("could not read %1").arg(filePath));
    return file.readAll();
}

// Opens a project through the main window, the same path as clicking it in
// the recent-projects list.
bool openInWindow(MainWindow& window, const QString& filePath)
{
    QListWidgetItem item;
    item.setData(Qt::UserRole, filePath);
    QMetaObject::invokeMethod(&window, "openRecentProject", Qt::DirectConnection,
                              Q_ARG(QListWidgetItem*, &item));
    QApplication::processEvents();
    ProjectManager* pm = Inst::get_project_manager();
    return pm->get_curr_project() != nullptr
        && QFileInfo(pm->fileinfo().absoluteFilePath())
               == QFileInfo(filePath);
}

// Saves the open project through the main window, like File > Save.
bool saveInWindow(MainWindow& window)
{
    bool saved = false;
    QMetaObject::invokeMethod(&window, "saveFile", Qt::DirectConnection,
                              Q_RETURN_ARG(bool, saved));
    QApplication::processEvents();
    return saved;
}

// Number of envelopes in a project file's <EnvelopeLibrary> (each starts
// with an "Envelope <n>" line).
int envelopesInFile(const QString& filePath)
{
    QFile file(filePath);
    require(file.open(QIODevice::ReadOnly),
            QStringLiteral("could not read %1").arg(filePath));
    QDomDocument document;
    require(static_cast<bool>(document.setContent(&file)),
            QStringLiteral("%1 is not valid XML").arg(filePath));
    const QString library = document.documentElement()
                                .firstChildElement(QStringLiteral("EnvelopeLibrary")).text();
    return static_cast<int>(library.split(QLatin1Char('\n'))
                                .filter(QRegularExpression(QStringLiteral("^\\s*Envelope\\b")))
                                .size());
}

// Copies a project, giving the copy Windows (CRLF) line endings.
bool copyWithWindowsLineEndings(const QString& from, const QString& to)
{
    QFile in(from);
    QFile out(to);
    if (!in.open(QIODevice::ReadOnly) || !out.open(QIODevice::WriteOnly))
        return false;
    QByteArray text = in.readAll();
    text.replace("\r\n", "\n");
    text.replace("\n", "\r\n");
    return out.write(text) == text.size();
}

// The round trip for one project file, in a folder whose name has spaces, an
// accent and Chinese characters. With windowsLineEndings, it starts from a
// copy with CRLF line endings, and the values must still match the original.
void roundTripCopy(MainWindow& window, const QString& originalPath, bool windowsLineEndings)
{
    QTemporaryDir workDir(QDir::tempPath()
                          + QStringLiteral("/LASSIE test Zoë 音乐-XXXXXX"));
    require(workDir.isValid(), QStringLiteral("could not create a temporary folder"));
    const QString copyPath = workDir.filePath(QFileInfo(originalPath).fileName());
    require(windowsLineEndings ? copyWithWindowsLineEndings(originalPath, copyPath)
                               : QFile::copy(originalPath, copyPath),
            QStringLiteral("could not copy %1 to a temporary folder").arg(originalPath));
    QFile::setPermissions(copyPath, QFile::ReadOwner | QFile::WriteOwner);

    require(openInWindow(window, copyPath),
            QStringLiteral("LASSIE could not open the project"));
    // Every envelope must be loaded, not only kept as text.
    EnvelopeLibraryEntry* entries = Inst::get_project_manager()->envlibentries();
    const int loaded = entries ? entries->count() : 0;
    const int expected = envelopesInFile(originalPath);
    if (loaded != expected)
        fail(QStringLiteral("%1 of %2 envelopes loaded").arg(loaded).arg(expected));
    require(saveInWindow(window), QStringLiteral("first save reported failure"));
    const QByteArray firstSave = readAll(copyPath);

    // Nothing from the original file may be lost or changed by saving.
    const QStringList original = flattenFile(originalPath);
    const QStringList saved = flattenFile(copyPath);
    const QSet<QString> savedSet(saved.cbegin(), saved.cend());
    int lost = 0;
    for (const QString& line : original) {
        if (!savedSet.contains(line)) {
            if (lost < 15)
                fail(QStringLiteral("lost or changed by saving: %1").arg(line));
            ++lost;
        }
    }
    if (lost > 15)
        fail(QStringLiteral("... and %1 more lost or changed values").arg(lost - 15));

    // Reopening the saved file and saving again must not change anything.
    require(openInWindow(window, copyPath),
            QStringLiteral("LASSIE could not reopen the file it just saved"));
    require(saveInWindow(window), QStringLiteral("second save reported failure"));
    const QByteArray secondSave = readAll(copyPath);
    if (secondSave != firstSave) {
        // Report which values differ, not just that the bytes differ.
        QTemporaryDir compareDir;
        const QString firstPath = compareDir.filePath(QStringLiteral("first.dissco"));
        QFile firstFile(firstPath);
        if (firstFile.open(QIODevice::WriteOnly)) {
            firstFile.write(firstSave);
            firstFile.close();
        }
        const QStringList a = flattenFile(firstPath);
        const QStringList b = flattenFile(copyPath);
        const QSet<QString> aSet(a.cbegin(), a.cend());
        const QSet<QString> bSet(b.cbegin(), b.cend());
        int reported = 0;
        for (const QString& line : a)
            if (!bSet.contains(line) && reported++ < 10)
                fail(QStringLiteral("changed by re-saving (was): %1").arg(line));
        for (const QString& line : b)
            if (!aSet.contains(line) && reported++ < 20)
                fail(QStringLiteral("changed by re-saving (now): %1").arg(line));
        if (reported == 0)
            fail(QStringLiteral("re-saving changed the file's formatting, though no values changed"));
    }
}

void roundTrip(MainWindow& window, const QString& originalPath)
{
    roundTripCopy(window, originalPath, false);
}

void roundTripWindowsLineEndings(MainWindow& window, const QString& originalPath)
{
    roundTripCopy(window, originalPath, true);
}

// Makes a folder read-only for as long as this object lives, then makes it
// writable again so it can be removed.
//
// Bug fix: on Windows, the read-only flag that QFile::setPermissions sets on a
// folder doesn't stop anyone from writing files into it, so the read-only
// folder check passed on Windows without testing anything. On Windows a "deny
// write" permission rule is now added with icacls instead. Since a user with
// admin rights (or root on macOS/Linux) can still write anywhere, isReadOnly()
// tries to write a probe file to confirm the folder really is read-only.
// -- Tony Chai, 2026
class ReadOnlyFolder {
public:
    explicit ReadOnlyFolder(const QString& path) : path_(path)
    {
#ifdef Q_OS_WIN
        // S-1-1-0 is "Everyone". (OI)(CI) also covers the files in the folder.
        // Only the write rights are denied: icacls's plain (W) also includes
        // the right to wait on a file, which Windows needs to read it.
        runIcacls({QStringLiteral("/deny"),
                   QStringLiteral("*S-1-1-0:(OI)(CI)(WD,AD,WEA,WA)")});
#else
        writable_ = QFile::permissions(path_);
        QFile::setPermissions(path_, QFile::ReadOwner | QFile::ExeOwner);
#endif
    }

    ~ReadOnlyFolder()
    {
#ifdef Q_OS_WIN
        runIcacls({QStringLiteral("/remove:d"), QStringLiteral("*S-1-1-0")});
#else
        QFile::setPermissions(path_, writable_);
#endif
    }

    bool isReadOnly() const
    {
        QFile probe(QDir(path_).filePath(QStringLiteral("write-probe.tmp")));
        if (!probe.open(QIODevice::WriteOnly))
            return true;
        probe.close();
        probe.remove();
        return false;
    }

private:
#ifdef Q_OS_WIN
    void runIcacls(const QStringList& change) const
    {
        QProcess::execute(QStringLiteral("icacls"),
                          QStringList{QDir::toNativeSeparators(path_)} << change
                              << QStringLiteral("/Q"));
    }
#else
    QFile::Permissions writable_;
#endif
    QString path_;
};

// A project opened from a folder LASSIE can't write to (read-only, a shared
// drive) must still come with all of its envelopes.
void openFromReadOnlyFolder(MainWindow& window, const QString& originalPath)
{
    const int expected = envelopesInFile(originalPath);
    QTemporaryDir workDir;
    require(workDir.isValid(), QStringLiteral("could not create a temporary folder"));
    const QString copyPath = workDir.filePath(QFileInfo(originalPath).fileName());
    require(QFile::copy(originalPath, copyPath),
            QStringLiteral("could not copy %1 to a temporary folder").arg(originalPath));

    bool opened = false;
    int loaded = 0;
    {
        ReadOnlyFolder readOnly(workDir.path());
        if (!readOnly.isReadOnly()) {
            // Passing here would prove nothing, so say so instead.
            std::cerr << "SKIPPED [" << currentTest << "]: this account can write to a "
                         "read-only folder (running as root or administrator?), so the "
                         "check can't be done here\n";
            currentTestSkipped = true;
            return;
        }
        opened = openInWindow(window, copyPath);
        EnvelopeLibraryEntry* entries = Inst::get_project_manager()->envlibentries();
        loaded = (opened && entries) ? entries->count() : 0;
    }  // the folder is writable again here, so it can be removed

    require(opened, QStringLiteral("LASSIE could not open the project from a read-only folder"));
    if (loaded != expected)
        fail(QStringLiteral("opened from a read-only folder: %1 of %2 envelopes loaded")
                 .arg(loaded).arg(expected));
}

void runTest(const std::string& name, void (*test)(MainWindow&, const QString&),
             MainWindow& window, const QString& path, std::vector<std::string>& failedTests)
{
    currentTest = name;
    currentTestFailures = 0;
    currentTestSkipped = false;
    std::cerr << "RUN  " << name << '\n';
    try {
        test(window, path);
    } catch (const RequirementFailed& e) {
        fail(e.message + QStringLiteral(" (stopped this test; later checks depend on it)"));
    } catch (const std::exception& e) {
        fail(QStringLiteral("unexpected exception: %1").arg(e.what()));
    } catch (...) {
        fail(QStringLiteral("unexpected non-standard exception"));
    }
    if (currentTestFailures > 0)
        failedTests.push_back(name);
    else if (!currentTestSkipped)
        std::cerr << "OK   " << name << '\n';
}

} // namespace

int runProjectRoundTripTest()
{
    // Keep the test's recent-projects list and settings out of the user's own.
    QCoreApplication::setOrganizationName(QStringLiteral("DISSCO-Tests"));
    QCoreApplication::setApplicationName(QStringLiteral("LASSIE-ProjectRoundTrip"));
    QSettings().clear();

    const QStringList folders = qEnvironmentVariable("DISSCO_TEST_PROJECT_DIRS")
                                    .split(QLatin1Char('|'), Qt::SkipEmptyParts);
    QStringList projects;
    for (const QString& folder : folders) {
        for (const QFileInfo& info :
             QDir(folder).entryInfoList({QStringLiteral("*.dissco")}, QDir::Files,
                                        QDir::Name))
            projects << info.absoluteFilePath();
    }
    if (projects.isEmpty()) {
        std::cerr << "FAILED: no .dissco files found in DISSCO_TEST_PROJECT_DIRS\n";
        return 1;
    }

    std::vector<std::string> failedTests;
    {
        MainWindow window(Inst::instance());
        DialogWatchdog watchdog;
        for (const QString& project : projects) {
            const std::string fileName = QFileInfo(project).fileName().toStdString();
            runTest("roundTrip(" + fileName + ")", roundTrip, window, project, failedTests);
            runTest("openFromReadOnlyFolder(" + fileName + ")", openFromReadOnlyFolder,
                    window, project, failedTests);
            runTest("roundTripWindowsLineEndings(" + fileName + ")",
                    roundTripWindowsLineEndings, window, project, failedTests);
        }
    }
    QSettings().clear();

    if (!failedTests.empty()) {
        std::cerr << '\n' << failedTests.size() << " project round-trip test(s) failed:\n";
        for (const std::string& name : failedTests)
            std::cerr << "  " << name << '\n';
        return 1;
    }
    std::cerr << "\nAll project round-trip tests passed (" << projects.size()
              << " projects)\n";
    return 0;
}
