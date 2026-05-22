#include "InverseFunction.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

InverseFunction::InverseFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(tr("Number To Inverse:"), this));

    m_entryEdit = new QLineEdit(this);
    layout->addWidget(m_entryEdit);

    auto* insertFnButton = new QPushButton(tr("Insert Function"), this);
    layout->addWidget(insertFnButton);

    connect(m_entryEdit, &QLineEdit::textChanged,
            this, &FunctionWidget::xmlChanged);

    connect(insertFnButton, &QPushButton::clicked, this, [this]() {
        // Capture the line edit by pointer; the callback will fire on the
        // FunctionGenerator side after the nested dialog is accepted.
        QLineEdit* target = m_entryEdit;
        emit nestedFunctionRequested(
            FunctionReturnType::functionReturnFloat,
            [target](const QString& result) {
                if (!result.isEmpty()) target->setText(result);
            });
    });
}

QList<FunctionReturnType> InverseFunction::supportedReturnTypes() const {
    // Matches the legacy combo box population: Inverse appears in both
    // the Int and Float pickers.
    return {
        FunctionReturnType::functionReturnInt,
        FunctionReturnType::functionReturnFloat,
    };
}

QString InverseFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>Inverse</Name><Entry>")
         + m_entryEdit->text()
         + QStringLiteral("</Entry></Fun>");
}

void InverseFunction::populateFromXML(QXmlStreamReader& reader) {
    // Caller has already consumed <Name>Inverse</Name>; the next child
    // is <Entry>X</Entry>.
    m_entryEdit->setText(nextChildInner(reader));
}

void InverseFunction::reset() {
    m_entryEdit->clear();
}
