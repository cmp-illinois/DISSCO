#include "FibonacciFunction.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

FibonacciFunction::FibonacciFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(tr("Fibonacci:"), this));

    m_entryEdit = new QLineEdit(this);
    layout->addWidget(m_entryEdit);

    auto* insertFnButton = new QPushButton(tr("Insert Function"), this);
    layout->addWidget(insertFnButton);

    connect(m_entryEdit, &QLineEdit::textChanged,
            this, &FunctionWidget::xmlChanged);

    connect(insertFnButton, &QPushButton::clicked, this, [this]() {
        QLineEdit* target = m_entryEdit;
        emit nestedFunctionRequested(
            FunctionReturnType::functionReturnFloat,
            [target](const QString& result) {
                if (!result.isEmpty()) target->setText(result);
            });
    });
}

QList<FunctionReturnType> FibonacciFunction::supportedReturnTypes() const {
    return {
        FunctionReturnType::functionReturnInt,
        FunctionReturnType::functionReturnFloat,
        FunctionReturnType::functionReturnMakeListFun,
    };
}

QString FibonacciFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>Fibonacci</Name><Entry>")
         + m_entryEdit->text()
         + QStringLiteral("</Entry></Fun>");
}

void FibonacciFunction::populateFromXML(QXmlStreamReader& reader) {
    m_entryEdit->setText(nextChildInner(reader));
}

void FibonacciFunction::reset() {
    m_entryEdit->clear();
}
