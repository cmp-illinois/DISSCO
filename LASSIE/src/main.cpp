#include "inst.hpp"

#include <QApplication>
#include <QFile>
#include <QIcon>
#ifdef DISSCO_ENABLE_UI_LAYOUT_TESTS
#include <QDebug>
#include <QFontDatabase>
#include <QFontInfo>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#ifdef Q_OS_WIN
#ifndef NOGDI
#define NOGDI
#endif
#include <qt_windows.h>
#endif
#include "widgets/EventAttributesViewController.hpp"
#include "widgets/Modifiers.hpp"
#include "windows/EnvelopeLibraryWindow.hpp"
#include "../tests/ProjectRoundTripTest.hpp"
#endif
#include "widgets/ComboBoxWheelGuard.hpp"
#include "widgets/TextOverflowDisplayPolicy.hpp"
#include "widgets/WindowShortcutPolicy.hpp"
#include "windows/MainWindow.hpp"

void registerAllFunctions();

int main(int argc, char *argv[])
{
    // QSettings (used here and in the Updater) needs stable org/app
    // metadata; otherwise its on-disk location is platform-default and
    // varies between binaries. Set before QApplication is constructed.
    QCoreApplication::setOrganizationName("DISSCO");
    QCoreApplication::setOrganizationDomain("dissco.illinois.edu");
    QCoreApplication::setApplicationName("LASSIE");

    QApplication a(argc, argv);
#ifndef Q_OS_MACOS
    a.setWindowIcon(QIcon(QStringLiteral(":/icons/LASSIE.png")));
#endif
#ifdef Q_OS_LINUX
    a.setDesktopFileName(QStringLiteral("LASSIE"));
#endif
    ComboBoxWheelGuard comboBoxWheelGuard(a);
    TextOverflowDisplayPolicy textOverflowDisplayPolicy(a);
    WindowShortcutPolicy windowShortcutPolicy(a);

#ifdef DISSCO_ENABLE_UI_LAYOUT_TESTS
#ifdef Q_OS_WIN
    if (qEnvironmentVariableIsSet("DISSCO_TEST_WINDOWS_FONT")) {
        const QFont font = a.font();
        if (!QFontDatabase::families().contains(font.family(), Qt::CaseInsensitive)) {
            qWarning() << "Skipping font check: system font uses an unlisted alias"
                       << font.family();
            return 77;
        }
        const QString resolvedFamily = QFontInfo(font).family();
        if (font.family().compare(resolvedFamily, Qt::CaseInsensitive) != 0) {
            qCritical() << "Installed Windows system font was substituted:"
                        << "requested" << font.family() << "resolved" << resolvedFamily;
            return 1;
        }
        return 0;
    }
#endif

    if (qEnvironmentVariableIsSet("DISSCO_TEST_APPLICATION_IDENTITY")) {
        EnvelopeLibraryWindow window;
        if (a.windowIcon().pixmap(32, 32).isNull()
            || window.windowIcon().pixmap(32, 32).isNull()) {
            qCritical() << "Application and top-level windows must have a usable icon";
            return 1;
        }
#ifdef Q_OS_WIN
        if (!FindResourceW(nullptr, MAKEINTRESOURCEW(1), RT_GROUP_ICON)) {
            qCritical() << "Executable is missing its Windows application icon resource";
            return 1;
        }
#endif
#ifdef Q_OS_LINUX
        if (a.desktopFileName() != QStringLiteral("LASSIE")) {
            qCritical() << "Desktop identity must match LASSIE.desktop";
            return 1;
        }
#endif
        return 0;
    }

    if (qEnvironmentVariableIsSet("DISSCO_TEST_ENVELOPE_LAYOUT")) {
        EnvelopeLibraryWindow window;
        window.ensurePolished();
        window.show();
        for (int i = 0; i < 3; ++i) {
            window.centralWidget()->layout()->activate();
            QApplication::processEvents();
        }

        QLabel* legend = window.findChild<QLabel*>("envelopeLegend");
        if (!legend) {
            qCritical() << "Envelope legend was not found";
            return 1;
        }

        const QRect legendInCentral(
            legend->mapTo(window.centralWidget(), QPoint()), legend->size());
        const int requiredHeight = legend->heightForWidth(legend->width());
        const bool fullyVisible =
            window.centralWidget()->rect().contains(legendInCentral)
            && legend->visibleRegion().contains(legend->rect())
            && legend->height() >= requiredHeight;

        if (!fullyVisible) {
            qCritical() << "Envelope legend is clipped:"
                        << "actual" << legend->size()
                        << "required height" << requiredHeight
                        << "visible region" << legend->visibleRegion();
        }

        return fullyVisible ? 0 : 1;
    }

    // Regression test for a modifier row's move-up/move-down buttons going
    // stale after a later modifier is added -- Tony Chai, 2026.
    if (qEnvironmentVariableIsSet("DISSCO_TEST_MODIFIER_ADD_REFRESH")) {
        ProjectManager* projectManager = Inst::get_project_manager();
        projectManager->set_curr_project(projectManager->create());

        EventAttributesViewController controller(nullptr);
        controller.show();
        controller.showAttributesOfEvent(top, 0);

        QPushButton* addButton =
            controller.findChild<QPushButton*>("addModifierButton");
        if (!addButton) {
            qCritical() << "addModifierButton was not found";
            return 1;
        }

        bool allCorrect = true;
        for (int addCount = 1; addCount <= 3; ++addCount) {
            addButton->click();
            // rebuildModifierRows() deleteLater()s the old rows -- flush
            // that before counting, or the about-to-be-destroyed ones are
            // still findable as children for one more event loop turn.
            QApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QApplication::processEvents();

            const QList<Modifiers*> rows =
                controller.findChildren<Modifiers*>();
            if (rows.size() != addCount) {
                qCritical() << "Expected" << addCount
                            << "modifier rows after add #" << addCount
                            << ", got" << rows.size();
                allCorrect = false;
                break;
            }
            for (int i = 0; i < rows.size(); ++i) {
                QPushButton* moveUp =
                    rows[i]->findChild<QPushButton*>("moveUpButton");
                QPushButton* moveDown =
                    rows[i]->findChild<QPushButton*>("moveDownButton");
                if (!moveUp || !moveDown) {
                    qCritical() << "Row" << i << "is missing move buttons";
                    allCorrect = false;
                    continue;
                }
                const bool expectUpEnabled = i > 0;
                const bool expectDownEnabled = i + 1 < rows.size();
                if (moveUp->isEnabled() != expectUpEnabled
                    || moveDown->isEnabled() != expectDownEnabled) {
                    qCritical()
                        << "Row" << i << "after add #" << addCount
                        << "has stale move-button state: up enabled ="
                        << moveUp->isEnabled() << "(expected"
                        << expectUpEnabled << "), down enabled ="
                        << moveDown->isEnabled() << "(expected"
                        << expectDownEnabled << ")";
                    allCorrect = false;
                }
            }
        }

        return allCorrect ? 0 : 1;
    }
#endif

    registerAllFunctions();
#ifdef DISSCO_ENABLE_UI_LAYOUT_TESTS
    // Save -> reopen round trip of real projects (needs the function
    // registry above, so it runs here rather than with the tests above).
    // -- Tony Chai, 2026
    if (qEnvironmentVariableIsSet("DISSCO_TEST_PROJECT_ROUNDTRIP"))
        return runProjectRoundTripTest();
#endif
    Inst *m = Inst::instance();
    MainWindow *w = new MainWindow(m);
    w->show();
    return a.exec();
}
