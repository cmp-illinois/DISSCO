#include "ResultStringWrapTest.hpp"

#include "../src/dialogs/FunctionGenerator.hpp"
#include "../src/dialogs/PartialModifierDialog.hpp"
#include "../src/widgets/Select.hpp"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QKeyEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextLayout>

namespace {

QString selectXml(int values)
{
    QStringList list;
    for (int i = 0; i < values; ++i)
        list << QStringLiteral("60");
    return QStringLiteral("<Fun><Name>Select</Name><List>%1</List><Index>0</Index></Fun>")
        .arg(list.join(QLatin1Char(',')));
}

void settle()
{
    for (int i = 0; i < 4; ++i)
        QApplication::processEvents();
}

void pressKey(QWidget* target, int key, const QString& text = QString())
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, text);
    QApplication::sendEvent(target, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, text);
    QApplication::sendEvent(target, &release);
}

void saveScreenshot(QWidget* widget, const QString& name)
{
    const QString directory = qEnvironmentVariable("DISSCO_TEST_SCREENSHOT_DIR");
    if (!directory.isEmpty())
        widget->grab().save(QDir(directory).filePath(name));
}

// Lines of the wrapped text, as laid out in the box.
QList<QTextLine> wrappedLines(const QTextEdit* box)
{
    QList<QTextLine> lines;
    for (QTextBlock block = box->document()->begin(); block.isValid(); block = block.next()) {
        const QTextLayout* layout = block.layout();
        for (int i = 0; i < layout->lineCount(); ++i)
            lines << layout->lineAt(i);
    }
    return lines;
}

} // namespace

int runResultStringWrapTest()
{
    bool passed = true;
    const auto check = [&passed](bool condition, const QString& message) {
        if (!condition) { qCritical().noquote() << message; passed = false; }
        return condition;
    };

    // The box wraps the text at its edge, at any character, and is tall
    // enough to show all of it.
    const auto checkWrapsAndShowsAll = [&](const QTextEdit* box, const QString& what) {
        const QList<QTextLine> lines = wrappedLines(box);
        check(lines.size() > 1, what + ": long text must wrap onto several lines");
        check(box->horizontalScrollBar()->maximum() == 0,
              what + ": the box must not scroll sideways");
        const qreal charWidth = box->fontMetrics().horizontalAdvance(QLatin1Char('W'));
        for (qsizetype i = 0; i + 1 < lines.size(); ++i) {
            // A line that breaks only at word boundaries would end early.
            check(lines[i].naturalTextWidth() > lines[i].width() - 2 * charWidth,
                  what + QStringLiteral(": line %1 must reach the edge of the box").arg(i + 1));
        }
        check(box->verticalScrollBar()->maximum() == 0,
              what + ": the whole text must be visible without scrolling");
        QTextCursor end(box->document());
        end.movePosition(QTextCursor::End);
        check(box->viewport()->rect().contains(box->cursorRect(end)),
              what + ": the end of the text must be inside the box");
    };

    // The box and the buttons below it must fit inside the dialog.
    const auto checkInsideDialog = [&](QDialog& dialog, const QTextEdit* box, const QString& what) {
        const QRect boxRect(box->mapTo(&dialog, QPoint()), box->size());
        auto* buttons = dialog.findChild<QDialogButtonBox*>();
        const QRect buttonRect(buttons->mapTo(&dialog, QPoint()), buttons->size());
        check(dialog.rect().contains(boxRect) && dialog.rect().contains(buttonRect),
              what + ": the box and the OK button must be inside the dialog");
        check(boxRect.bottom() < buttonRect.top(), what + ": the box must not cover the buttons");
        check(dialog.height() >= dialog.layout()->totalMinimumSize().height(),
              what + ": the dialog must be tall enough for all its fields");
    };

    // 1. A long Select string, as in Sever's report.
    const QString longXml = selectXml(120);
    FunctionGenerator dialog(nullptr, FunctionReturnType::functionReturnFloat, longXml);
    dialog.show();
    settle();
    auto* box = dialog.findChild<QTextEdit*>("resultTextEdit");
    if (!check(box != nullptr, "Result String box is missing")) return 1;
    check(box->toPlainText() == longXml, "Opening the dialog must show the string unchanged");
    // First at the size the dialog opens with, then at the width of the report.
    checkWrapsAndShowsAll(box, "Long string at opening size");
    checkInsideDialog(dialog, box, "Long string at opening size");
    saveScreenshot(&dialog, "result-string-long-opening-size.png");
    dialog.resize(900, dialog.height());
    settle();
    checkWrapsAndShowsAll(box, "Long string");
    checkInsideDialog(dialog, box, "Long string");
    saveScreenshot(&dialog, "result-string-long.png");

    // A short string keeps a small box; the long one made it taller.
    FunctionGenerator shortDialog(nullptr, FunctionReturnType::functionReturnFloat, selectXml(2));
    shortDialog.resize(900, shortDialog.height());
    shortDialog.show();
    settle();
    auto* shortBox = shortDialog.findChild<QTextEdit*>("resultTextEdit");
    check(shortBox->height() < box->height(), "The box must grow for a longer string");
    check(wrappedLines(shortBox).size() == 1, "A short string must stay on one line");
    saveScreenshot(&shortDialog, "result-string-short.png");
    shortDialog.close();

    // 2. A wider dialog re-wraps the text into fewer lines.
    const int linesBefore = wrappedLines(box).size();
    dialog.resize(dialog.width() + 300, dialog.height());
    settle();
    check(wrappedLines(box).size() < linesBefore, "A wider box must re-wrap into fewer lines");
    checkWrapsAndShowsAll(box, "Wider dialog");
    checkInsideDialog(dialog, box, "Wider dialog");
    dialog.resize(900, dialog.height());
    settle();

    // 3. Changing a field still rewrites the Result String, and the box
    //    grows to show the longer result.
    const QList<Select*> nodes = dialog.findChildren<Select*>();
    if (check(nodes.size() == 120, "Select rows were not loaded from the string")) {
        const int heightBefore = box->height();
        nodes.first()->setValText(QStringLiteral("<Fun><Name>Random</Name><LowBound>40</LowBound>"
                                                 "<HighBound>80</HighBound></Fun>"));
        settle();
        check(box->toPlainText().contains(QStringLiteral("<List><Fun><Name>Random</Name>")),
              "Changing a field must update the Result String");
        check(box->height() > heightBefore, "The box must grow when a field makes the string longer");
        checkWrapsAndShowsAll(box, "After a field change");
        checkInsideDialog(dialog, box, "After a field change");
    }

    // 4. Sever edits a value in the box itself, then presses Enter and OK.
    box->setPlainText(longXml);
    settle();
    box->setFocus();
    QTextCursor cursor(box->document());
    const int listStart = longXml.indexOf(QStringLiteral("<List>")) + 6;
    cursor.setPosition(listStart);
    box->setTextCursor(cursor);
    pressKey(box, Qt::Key_Delete);
    pressKey(box, Qt::Key_Delete);
    pressKey(box, Qt::Key_7, QStringLiteral("7"));
    pressKey(box, Qt::Key_2, QStringLiteral("2"));
    QString editedXml = longXml;
    editedXml.replace(listStart, 2, QStringLiteral("72"));
    check(box->toPlainText() == editedXml, "Typing in the box must change the string");
    pressKey(box, Qt::Key_Return);
    pressKey(box, Qt::Key_Enter);
    check(!box->toPlainText().contains(QLatin1Char('\n')), "Enter must not add a line break");
    check(dialog.isVisible() && dialog.result() != QDialog::Accepted,
          "Enter in the box must not close the dialog");
    dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    check(dialog.result() == QDialog::Accepted, "OK must accept the dialog");
    check(dialog.getResultString() == editedXml, "OK must return the string as edited in the box");

    // 5. Switching functions resizes the dialog; the box must still fit.
    {
        FunctionGenerator switching(nullptr, FunctionReturnType::functionReturnFloat, longXml);
        switching.show();
        settle();
        auto* switchingBox = switching.findChild<QTextEdit*>("resultTextEdit");
        auto* options = switching.findChild<QComboBox*>("functionOptions");
        for (const char* name : {"Inverse", "Select", "Random", "Select"}) {
            options->setCurrentIndex(options->findText(QString::fromLatin1(name)));
            settle();
            const QString what = QStringLiteral("After switching to %1").arg(QString::fromLatin1(name));
            check(switchingBox->verticalScrollBar()->maximum() == 0,
                  what + ": the whole text must be visible without scrolling");
            checkInsideDialog(switching, switchingBox, what);
        }
        checkWrapsAndShowsAll(switchingBox, "After switching back to Select");
    }

    // 6. Pasting a long string into an empty dialog grows the box as well.
    //    The empty dialog is narrow, so use a string that fits in 12 lines.
    FunctionGenerator empty(nullptr, FunctionReturnType::functionReturnFloat);
    empty.show();
    settle();
    auto* emptyBox = empty.findChild<QTextEdit*>("resultTextEdit");
    const int emptyHeight = emptyBox->height();
    const QString pastedXml = selectXml(60);
    emptyBox->insertPlainText(pastedXml);
    settle();
    check(emptyBox->height() > emptyHeight, "The box must grow when a long string is pasted");
    checkWrapsAndShowsAll(emptyBox, "Pasted into an empty dialog");
    checkInsideDialog(empty, emptyBox, "Pasted into an empty dialog");
    check(empty.getResultString() == pastedXml, "A pasted string must be returned unchanged");
    empty.close();

    // 7. A very long string fills the box to its limit and scrolls.
    FunctionGenerator veryLong(nullptr, FunctionReturnType::functionReturnFloat, selectXml(2000));
    veryLong.resize(900, veryLong.height());
    veryLong.show();
    settle();
    auto* veryLongBox = veryLong.findChild<QTextEdit*>("resultTextEdit");
    check(veryLongBox->height() > box->height(), "A very long string must use a taller box");
    check(veryLongBox->verticalScrollBar()->maximum() > 0,
          "Only a very long string should need to scroll");
    check(veryLongBox->horizontalScrollBar()->maximum() == 0,
          "A very long string must still wrap instead of scrolling sideways");
    checkInsideDialog(veryLong, veryLongBox, "Very long string");
    saveScreenshot(&veryLong, "result-string-very-long.png");
    veryLong.close();

    // 8. The Partial Result String preview wraps the same way.
    PartialModifierDialog partials(nullptr, 0, {4, 0, QString()});
    partials.show();
    settle();
    auto* addPartial = partials.findChild<QPushButton*>("addPartialButton");
    if (check(addPartial != nullptr, "Add partial button is missing")) {
        addPartial->click();
        settle();
    }
    auto* preview = partials.findChild<QTextEdit*>("partialResultPreview");
    if (check(preview != nullptr, "Partial Result String preview is missing")) {
        check(preview->isReadOnly(), "The partial preview must stay read-only");
        checkWrapsAndShowsAll(preview, "Partial preview");
        saveScreenshot(&partials, "partial-result-string.png");
    }
    partials.close();

    return passed ? 0 : 1;
}
