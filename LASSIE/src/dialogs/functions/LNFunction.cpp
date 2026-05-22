#include "LNFunction.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

LNFunction::LNFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(tr("LN:"), this));

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

QList<FunctionReturnType> LNFunction::supportedReturnTypes() const {
    return {
        FunctionReturnType::functionReturnFloat,
        FunctionReturnType::functionReturnMakeListFun,
    };
}

QString LNFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>LN</Name><Entry>")
         + m_entryEdit->text()
         + QStringLiteral("</Entry></Fun>");
}

void LNFunction::populateFromXML(QXmlStreamReader& reader) {
    m_entryEdit->setText(nextChildInner(reader));
}

void LNFunction::reset() {
    m_entryEdit->clear();
}
