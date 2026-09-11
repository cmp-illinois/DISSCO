#include "FunctionGenerator.hpp"
#include "FunctionXmlFormat.hpp"
#include "../core/RandomOrderIntId.hpp"
#include "../ui/ui_FunctionGenerator.h"

#include "functions/FunctionRegistry.hpp"
#include "functions/FunctionWidget.hpp"
#include "functions/impl/RandomOrderIntFunction.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QFontInfo>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVariant>
#include <QXmlStreamReader>

using enum CMODFunction;
using enum FunctionReturnType;

FunctionGenerator::FunctionGenerator(QWidget* parent,
                                     FunctionReturnType returnType,
                                     QString originalString)
    : QDialog(parent),
      ui(new Ui::FunctionGenerator),
      m_returnType(returnType),
      m_originalString(std::move(originalString))
{
    ui->setupUi(this);
    // The .ui's single placeholder page must not stretch the dialog.
    ui->functionStackedWidget->widget(0)->setSizePolicy(QSizePolicy::Ignored,
                                                        QSizePolicy::Ignored);
    ui->functionStackedWidget->setSizePolicy(QSizePolicy::Expanding,
                                              QSizePolicy::Expanding);
    layout()->setSizeConstraint(QLayout::SetDefaultConstraint);

    setupUi();
    adjustSize();
}

FunctionGenerator::~FunctionGenerator()
{
    delete ui;
}

QString FunctionGenerator::getResultString()
{
    const QString original = FunctionXmlFormat::compact(ui->resultTextEdit->toPlainText());
    m_result = FunctionXmlFormat::compact(RandomOrderIntId::repairMissing(original));
    if (m_result != original)
        ui->resultTextEdit->setPlainText(FunctionXmlFormat::preview(m_result));
    return m_result;
}

void FunctionGenerator::setupUi()
{
    m_originalString = RandomOrderIntId::repairMissing(m_originalString);
    // Populate the combo box from the registry, filtered by return type.
    auto& reg = FunctionRegistry::instance();
    ui->functionOptions->clear();
    ui->functionOptions->addItem(QString(), static_cast<int>(NOT_A_FUNCTION));
    for (CMODFunction id : reg.functionsFor(m_returnType))
        ui->functionOptions->addItem(reg.displayName(id), static_cast<int>(id));

    connect(ui->functionOptions, &QComboBox::currentIndexChanged,
            this, &FunctionGenerator::handleFunctionChanged);
    ui->functionOptions->setCurrentIndex(0);

    ui->resultLabel->setAlignment(Qt::AlignCenter);
    QFont resultFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    if (!QFontInfo(resultFont).fixedPitch()) {
        for (const QString& family : QFontDatabase::families()) {
            if (QFontDatabase::isFixedPitch(family)) {
                resultFont.setFamily(family);
                break;
            }
        }
    }
    resultFont.setStyleHint(QFont::Monospace);
    ui->resultTextEdit->setFont(resultFont);
    ui->resultTextEdit->setAcceptRichText(false);
    ui->resultTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    ui->resultTextEdit->setMinimumHeight(ui->resultTextEdit->fontMetrics().lineSpacing() * 3);
    ui->resultTextEdit->setMaximumHeight(ui->resultTextEdit->fontMetrics().lineSpacing() * 4);
    ui->resultTextEdit->setToolTip(tr("Function expression on one line. Scroll horizontally to read longer expressions. Use the fields above to edit parameters."));
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (m_originalString.isEmpty()) return;

    // Parse the incoming function string: <Fun><Name>X</Name>...</Fun>.
    // The matching FunctionWidget consumes the remaining children.
    QXmlStreamReader r(m_originalString);
    if (!r.readNextStartElement() || !r.readNextStartElement()) { // <Fun>, <Name>
        ui->resultTextEdit->setPlainText(FunctionXmlFormat::preview(m_originalString));
        return;
    }
    const QString functionName = FunctionWidget::readInner(r);

    const CMODFunction id = reg.idFromXmlName(functionName);
    if (id == NOT_A_FUNCTION) {
        ui->resultTextEdit->setPlainText(FunctionXmlFormat::preview(m_originalString));
        return;
    }
    FunctionWidget* w = ensureRegisteredWidget(id);
    if (!w) return;
    if (auto* randomOrder = qobject_cast<RandomOrderIntFunction*>(w))
        randomOrder->setOriginalXml(m_originalString);

    // Select the matching combo entry by stored id rather than by string,
    // to handle functions whose display name differs from their xml name
    // (e.g. Markov -> "GetFromMarkovChain").
    for (int i = 0; i < ui->functionOptions->count(); ++i) {
        if (static_cast<CMODFunction>(ui->functionOptions->itemData(i).toInt()) == id) {
            ui->functionOptions->setCurrentIndex(i);
            break;
        }
    }
    w->populateFromXML(r);
    // Populating the controls may emit xmlChanged, but opening the dialog
    // must not replace manual XML edits or fields unknown to the form.
    ui->resultTextEdit->setPlainText(FunctionXmlFormat::preview(m_originalString));
}

FunctionWidget* FunctionGenerator::ensureRegisteredWidget(CMODFunction id)
{
    if (auto it = m_registeredWidgets.find(id); it != m_registeredWidgets.end())
        return *it;
    FunctionWidget* w = FunctionRegistry::instance().create(id);
    if (!w) return nullptr;
    const int pageIndex = ui->functionStackedWidget->addWidget(w);
    m_registeredWidgets.insert(id, w);
    m_registeredPageIndex.insert(id, pageIndex);
    connect(w, &FunctionWidget::xmlChanged, this, [this, w]() {
        ui->resultTextEdit->setPlainText(FunctionXmlFormat::preview(w->buildXMLString()));
    });
    return w;
}

void FunctionGenerator::handleFunctionChanged(int index)
{
    QVariant functionData = ui->functionOptions->itemData(index);
    if (!functionData.isValid()) return;
    const CMODFunction id = static_cast<CMODFunction>(functionData.toInt());

    if (id == NOT_A_FUNCTION) {
        ui->resultTextEdit->clear();
        ui->functionStackedWidget->setCurrentIndex(0);
        resizeToFitCurrentPage();
        return;
    }

    if (FunctionWidget* w = ensureRegisteredWidget(id)) {
        ui->functionStackedWidget->setCurrentIndex(m_registeredPageIndex.value(id));
        ui->resultTextEdit->setPlainText(FunctionXmlFormat::preview(w->buildXMLString()));
        resizeToFitCurrentPage();
    }
}

void FunctionGenerator::resizeToFitCurrentPage()
{
    const int current = ui->functionStackedWidget->currentIndex();
    setMinimumSize(0, 0);
    for (int i = 0; i < ui->functionStackedWidget->count(); ++i) {
        ui->functionStackedWidget->widget(i)->setSizePolicy(
            i == current ? QSizePolicy::Expanding : QSizePolicy::Ignored,
            i == current ? QSizePolicy::Expanding : QSizePolicy::Ignored);
    }
    ui->functionStackedWidget->layout()->activate();
    layout()->activate();
    const QSize ideal = sizeHint();
    resize(ideal);
    setMinimumSize(ideal);
}
