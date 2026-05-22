#include "FunctionGenerator.hpp"
#include "../ui/ui_FunctionGenerator.h"

#include "functions/FunctionRegistry.hpp"
#include "functions/FunctionWidget.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
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
    m_result = ui->resultTextEdit->toPlainText();
    return m_result;
}

void FunctionGenerator::setupUi()
{
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
    ui->resultTextEdit->setMinimumHeight(60);
    ui->resultTextEdit->setMaximumHeight(100);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (m_originalString.isEmpty()) return;

    // Parse the incoming function string: <Fun><Name>X</Name>...</Fun>.
    // The matching FunctionWidget consumes the remaining children.
    QXmlStreamReader r(m_originalString);
    if (!r.readNextStartElement()) return;       // <Fun>
    if (!r.readNextStartElement()) return;       // <Name>
    const QString functionName = FunctionWidget::readInner(r);

    const CMODFunction id = reg.idFromXmlName(functionName);
    if (id == NOT_A_FUNCTION) return;
    FunctionWidget* w = ensureRegisteredWidget(id);
    if (!w) return;

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
        ui->resultTextEdit->setText(w->buildXMLString());
    });
    return w;
}

void FunctionGenerator::handleFunctionChanged(int index)
{
    QVariant data = ui->functionOptions->itemData(index);
    if (!data.isValid()) return;
    const CMODFunction id = static_cast<CMODFunction>(data.toInt());

    if (id == NOT_A_FUNCTION) {
        ui->resultTextEdit->clear();
        ui->functionStackedWidget->setCurrentIndex(0);
        resizeToFitCurrentPage();
        return;
    }

    if (FunctionWidget* w = ensureRegisteredWidget(id)) {
        ui->functionStackedWidget->setCurrentIndex(m_registeredPageIndex.value(id));
        ui->resultTextEdit->setText(w->buildXMLString());
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
