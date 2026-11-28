// Spatialization, reverb, filter and modifiers are hidden wherever they
// cannot change the output: with Sound Synthesis off, and on "n..." (note)
// Bottoms. Hidden values must survive saving.
#include "SoundOnlyFieldsTest.hpp"

#include "../src/core/project_struct.hpp"
#include "../src/dialogs/ProjectPropertiesDialog.hpp"
#include "../src/widgets/ModifierUiPolicy.hpp"
#include "../src/widgets/ProjectViewController.hpp"
#include "../src/windows/MainWindow.hpp"
#include "../src/ui/ui_ProjectPropertiesDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QScrollArea>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

int runSoundOnlyFieldsTest()
{
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message) {
        if (!condition) { qCritical() << message; passed = false; }
        return condition;
    };

    using ModifierUiPolicy::soundOnlyFieldsVisible;
    check(soundOnlyFieldsVisible(top, true, QString()),
          "Top must show the fields when Sound Synthesis is on");
    check(soundOnlyFieldsVisible(bottom, true, "s1"),
          "A sound Bottom must show the fields");
    check(!soundOnlyFieldsVisible(bottom, true, "n1"),
          "A note Bottom must hide the fields");
    for (Eventtype type : {top, high, mid, low, bottom}) {
        check(!soundOnlyFieldsVisible(type, false, "s1"),
              "Every level must hide the fields when Sound Synthesis is off");
    }

    QTemporaryDir directory;
    const QString filename = directory.filePath("sound_only.dissco");
    if (!check(QFile::copy(qEnvironmentVariable("DISSCO_TEST_PROJECT"), filename),
               "Cannot copy the test project")) return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    MainWindow window(Inst::instance());
    ProjectManager& manager = *Inst::get_project_manager();
    auto* project = manager.open(filename);
    if (!check(project && !manager.bottomevents().isEmpty()
                   && manager.bottomevents()[0].event.name.startsWith('s')
                   && manager.synthesis(),
               "The test project must open with a sound Bottom and synthesis on"))
        return 1;
    const QString savedSpa = manager.bottomevents()[0].extra_info.spa;
    if (!check(!savedSpa.isEmpty(), "The test project must set a spatialization"))
        return 1;

    const char* fieldNames[] = {
        "spaEntry", "revEntry", "filEntry", "modifiersLabel",
        "addModifierButton", "modifierUsageControls"};
    const auto fieldsShown = [&](bool expected) {
        for (const char* name : fieldNames) {
            auto* widget = window.findChild<QWidget*>(name);
            if (!widget || widget->isHidden() == expected)
                return false;
        }
        return true;
    };
    const auto setSynthesis = [&](ProjectView& view, bool on) {
        QTimer::singleShot(0, [&]() {
            auto* dialog = qobject_cast<ProjectPropertiesDialog*>(
                QApplication::activeModalWidget());
            if (!check(dialog != nullptr, "Project Properties did not open"))
                return;
            dialog->ui->synthesisCheckBox->setChecked(on);
            dialog->accept();
        });
        view.setProperties();
    };
    // Let pending deletions and layout passes finish, as the event loop would
    // in the running app.
    const auto settle = []() {
        for (int i = 0; i < 5; ++i) {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QApplication::processEvents();
        }
    };
    // Optional: save each state of the event page for a visual check.
    const QString screenshotDir = qEnvironmentVariable("DISSCO_TEST_SCREENSHOT_DIR");
    const auto snapshot = [&](const char* name) {
        if (screenshotDir.isEmpty())
            return;
        settle();
        auto* events = window.findChild<QScrollArea*>("eventsScrollArea");
        check(events && events->widget()->grab().save(
                  screenshotDir + "/" + name + ".png"),
              "Cannot save the event page rendering");
    };

    // After fields are hidden or shown in place, the event view's height must
    // follow the page, or the page shows blank gaps or loses its bottom rows.
    // The margin around the page stays constant when it does.
    const auto pageMargin = [&]() {
        settle();
        QWidget* eventView = window.findChild<QScrollArea*>("eventsScrollArea")->widget();
        QWidget* pages = eventView ? eventView->findChild<QWidget*>("stackedWidget") : nullptr;
        if (!check(pages != nullptr, "The event view is missing"))
            return -1;
        return eventView->height() - pages->sizeHint().height();
    };

    {
        ProjectView view(&window, filename);
        // Layout runs only for shown widgets (offscreen, so nothing appears).
        window.resize(1100, 1000);
        window.show();
        view.showAttributes("Bottom", 0);
        check(fieldsShown(true), "A sound Bottom must show the fields");
        const int margin = pageMargin();
        snapshot("1_sound_bottom");

        setSynthesis(view, false);
        check(!manager.synthesis(), "Project Properties did not turn synthesis off");
        check(fieldsShown(false),
              "Turning Sound Synthesis off must hide the fields on the open Bottom");
        check(pageMargin() == margin,
              "The event view must shrink when Sound Synthesis is turned off");
        snapshot("2_synthesis_off_bottom");
        view.showAttributes("Top", 0);
        check(fieldsShown(false), "Top must hide the fields when Sound Synthesis is off");
        snapshot("3_synthesis_off_top");

        setSynthesis(view, true);
        check(fieldsShown(true),
              "Turning Sound Synthesis on must show the fields on the open Top");
        check(pageMargin() == margin,
              "The event view must grow when Sound Synthesis is turned on");
        snapshot("4_sound_top");

        // Rename the way the palette does: update the project, then notify.
        view.showAttributes("Bottom", 0);
        manager.bottomevents()[0].event.name = "n1";
        view.updateAttributesNameEntry("Bottom", 0, "n1");
        check(fieldsShown(false), "Renaming a Bottom to n... must hide the fields");
        check(pageMargin() == margin,
              "The event view must shrink when a Bottom is renamed to n...");
        snapshot("5_note_bottom");

        // Opening another event can also show the fields again.
        view.showAttributes("Top", 0);
        check(fieldsShown(true), "Top must show the fields after a note Bottom");
        check(pageMargin() == margin,
              "The event view must fit Top when it is opened after a note Bottom");
        snapshot("6_top_after_note_bottom");
        view.showAttributes("Bottom", 0);
        check(fieldsShown(false), "Reopening the note Bottom must hide the fields");
        check(pageMargin() == margin,
              "The event view must fit a note Bottom opened after Top");

        check(view.save(), "Cannot save the test project");
    }
    manager.close(project);

    project = manager.open(filename);
    if (!check(project != nullptr, "Cannot reopen the saved project")) return 1;
    check(manager.bottomevents()[0].extra_info.spa == savedSpa,
          "Hidden spatialization must be kept when the project is saved");
    manager.close(project);
    return passed ? 0 : 1;
}
