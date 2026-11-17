// Checks what LASSIE's function picker (the dialog behind every "Fun" button)
// offers and writes. Each check reports its own failure and the rest still
// run. -- Tony Chai, 2026

#include "FunctionPickerTest.hpp"

#include "../src/dialogs/FunctionGenerator.hpp"

#include <QComboBox>
#include <QStringList>

#include <iostream>

namespace {

int failures = 0;

void check(bool passed, const QString& what)
{
    if (passed) {
        std::cerr << "ok      " << what.toStdString() << '\n';
    } else {
        ++failures;
        std::cerr << "FAILED  " << what.toStdString() << '\n';
    }
}

QComboBox* menuBox(FunctionGenerator& picker)
{
    return picker.findChild<QComboBox*>(QStringLiteral("functionOptions"));
}

// The names the picker lists, as the user sees them.
QStringList menu(FunctionGenerator& picker)
{
    QStringList names;
    if (QComboBox* box = menuBox(picker))
        for (int i = 0; i < box->count(); ++i)
            names << box->itemText(i);
    return names;
}

QString chosen(FunctionGenerator& picker)
{
    QComboBox* box = menuBox(picker);
    return box ? box->currentText() : QString();
}

const QString expDecay = QStringLiteral("EXP_DECAY: e^(-x)");

} // namespace

int runFunctionPickerTest()
{
    failures = 0;
    const QList<CMODFunction> startTimeExclusions = {
        CMODFunction::function_staticCURRENT_TYPE,
        CMODFunction::function_staticCURRENT_LAYER,
    };

    {
        FunctionGenerator picker(nullptr, FunctionReturnType::functionReturnFloat);
        const QStringList names = menu(picker);
        check(menuBox(picker) != nullptr, "the picker has its function menu");

        // Renamed: LN gives e^(-x), PREVIOUS_CHILD_DURATION gives an end time.
        check(names.contains(expDecay), "menu lists EXP_DECAY: e^(-x)");
        check(!names.contains("LN"), "menu no longer lists LN");
        check(names.contains("PREVIOUS_CHILD_END_TIME"), "menu lists PREVIOUS_CHILD_END_TIME");
        check(!names.contains("PREVIOUS_CHILD_DURATION"),
              "menu no longer lists PREVIOUS_CHILD_DURATION");

        // CURRENT_SEGMENT does nothing in CMOD, so it isn't offered.
        check(!names.contains("CURRENT_SEGMENT"), "menu no longer lists CURRENT_SEGMENT");

        // Outside a Start Time, the type functions are still offered.
        check(names.contains("CURRENT_TYPE") && names.contains("CURRENT_LAYER"),
              "an ordinary number picker lists CURRENT_TYPE and CURRENT_LAYER");

        // Choosing EXP_DECAY writes the new name.
        if (QComboBox* box = menuBox(picker)) {
            box->setCurrentIndex(box->findText(expDecay));
            check(picker.getResultString().contains("<Name>EXP_DECAY</Name>"),
                  "choosing EXP_DECAY writes <Name>EXP_DECAY</Name>, got: "
                      + picker.getResultString());
        }
    }

    // A child Start Time picker leaves out CURRENT_TYPE and CURRENT_LAYER,
    // and so does a picker opened from inside it (e.g. for a Select entry).
    {
        FunctionGenerator startTime(nullptr, FunctionReturnType::functionReturnFloat, QString(), startTimeExclusions);
        const QStringList names = menu(startTime);
        check(!names.contains("CURRENT_TYPE") && !names.contains("CURRENT_LAYER"),
              "Start Time picker leaves out CURRENT_TYPE and CURRENT_LAYER");
        check(names.contains("CURRENT_CHILD_NUM") && names.contains("Select"),
              "Start Time picker still lists the other functions");
        {
            FunctionGenerator inner(nullptr, FunctionReturnType::functionReturnFloat);
            const QStringList innerNames = menu(inner);
            check(!innerNames.contains("CURRENT_TYPE") && !innerNames.contains("CURRENT_LAYER"),
                  "a picker opened from the Start Time picker also leaves them out");
        }
    }
    {
        FunctionGenerator later(nullptr, FunctionReturnType::functionReturnFloat);
        check(menu(later).contains("CURRENT_TYPE"),
              "after the Start Time picker closes, other pickers list CURRENT_TYPE again");
    }

    // Fields saved with the old names still open on the renamed function.
    {
        FunctionGenerator oldLN(nullptr, FunctionReturnType::functionReturnFloat,
                                "<Fun><Name>LN</Name><Entry>2</Entry></Fun>");
        check(chosen(oldLN) == expDecay,
              "a field saved as LN opens as EXP_DECAY: e^(-x), got: " + chosen(oldLN));
        FunctionGenerator oldPrevious(nullptr, FunctionReturnType::functionReturnFloat,
                                      "<Fun><Name>PREVIOUS_CHILD_DURATION</Name></Fun>");
        check(chosen(oldPrevious) == "PREVIOUS_CHILD_END_TIME",
              "a field saved as PREVIOUS_CHILD_DURATION opens as PREVIOUS_CHILD_END_TIME, got: "
                  + chosen(oldPrevious));
    }

    // A field that already uses CURRENT_SEGMENT opens without losing it.
    {
        FunctionGenerator segment(nullptr, FunctionReturnType::functionReturnFloat,
                                  "<Fun><Name>CURRENT_SEGMENT</Name></Fun>");
        check(segment.getResultString().contains("<Name>CURRENT_SEGMENT</Name>"),
              "a field that uses CURRENT_SEGMENT keeps it, got: " + segment.getResultString());
    }

    std::cerr << (failures == 0 ? "\nAll function picker checks passed\n"
                                : "\n" + std::to_string(failures) + " function picker check(s) failed\n");
    return failures == 0 ? 0 : 1;
}
