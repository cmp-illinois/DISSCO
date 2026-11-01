// Save -> reopen round-trip test for LASSIE projects. -- Tony Chai, 2026
//
// For each project file, this drives LASSIE's real main window offscreen,
// like a user would: open it, save it, reopen the saved file, and save again.
// It then checks the *results*, not how LASSIE produced them:
//   - the project opens and saves without errors or pop-up dialogs,
//   - nothing in the original file is lost or changed by saving,
//   - saving an unchanged project a second time gives an identical file.
//
// Run through ctest (LASSIE.ProjectSaveRoundTrip). The project files used are
// the tutorial examples in Documents/examples/ plus
// LASSIE/tests/data/roundtrip_all_objects.dissco, which holds one of every
// kind of object a project can contain.

#include "ProjectRoundTripTest.hpp"

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

// The round trip for one project file.
void roundTrip(MainWindow& window, const QString& originalPath)
{
    QTemporaryDir workDir;
    require(workDir.isValid(), QStringLiteral("could not create a temporary folder"));
    const QString copyPath = workDir.filePath(QFileInfo(originalPath).fileName());
    require(QFile::copy(originalPath, copyPath),
            QStringLiteral("could not copy %1 to a temporary folder").arg(originalPath));
    QFile::setPermissions(copyPath, QFile::ReadOwner | QFile::WriteOwner);

    require(openInWindow(window, copyPath),
            QStringLiteral("LASSIE could not open the project"));
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

void runTest(const std::string& name, MainWindow& window, const QString& path,
             std::vector<std::string>& failedTests)
{
    currentTest = name;
    currentTestFailures = 0;
    std::cerr << "RUN  " << name << '\n';
    try {
        roundTrip(window, path);
    } catch (const RequirementFailed& e) {
        fail(e.message + QStringLiteral(" (stopped this test; later checks depend on it)"));
    } catch (const std::exception& e) {
        fail(QStringLiteral("unexpected exception: %1").arg(e.what()));
    } catch (...) {
        fail(QStringLiteral("unexpected non-standard exception"));
    }
    if (currentTestFailures > 0)
        failedTests.push_back(name);
    else
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
            const std::string name =
                "roundTrip(" + QFileInfo(project).fileName().toStdString() + ")";
            runTest(name, window, project, failedTests);
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
