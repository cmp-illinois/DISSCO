#include "Stochos.hpp"
#include "../inst.hpp"

using enum FunctionReturnType;

Stochos::Stochos(int methodType, int stochosIndex, QWidget *parent)
    : QFrame(parent),
      m_stochosIndex(stochosIndex),
      m_methodType(methodType)
{
    m_mainLayout = new QVBoxLayout(this);
    setFrameShape(QFrame::NoFrame);
    QHBoxLayout* stochosHBox = new QHBoxLayout();

    if (methodType == 0) {
        m_mainLayout->setContentsMargins(10, 10, 10, 10);
        m_mainLayout->setSpacing(10);
        auto* minLabel = new QLabel("Min:");
        m_minEntry = new QLineEdit;
        auto* maxLabel = new QLabel("Max:");
        m_maxEntry = new QLineEdit;
        auto* distLabel = new QLabel("Distribution:");
        m_distEntry = new QLineEdit;
        m_removeStochosButton = new QPushButton("Remove Node");

        connect(m_removeStochosButton, &QPushButton::clicked, this, &Stochos::onRemoveNodeClicked);
        connect(m_minEntry,  &QLineEdit::textChanged,           this, &Stochos::onNodeTextChanged);
        connect(m_maxEntry,  &QLineEdit::textChanged,           this, &Stochos::onNodeTextChanged);
        connect(m_distEntry, &QLineEdit::textChanged,           this, &Stochos::onNodeTextChanged);
        connect(m_minEntry,  &QLineEdit::cursorPositionChanged, this, [this](){ emit editFocused(m_minEntry); });
        connect(m_maxEntry,  &QLineEdit::cursorPositionChanged, this, [this](){ emit editFocused(m_maxEntry); });
        connect(m_distEntry, &QLineEdit::cursorPositionChanged, this, [this](){ emit editFocused(m_distEntry); });

        stochosHBox->addWidget(minLabel);
        stochosHBox->addWidget(m_minEntry);
        stochosHBox->addWidget(maxLabel);
        stochosHBox->addWidget(m_maxEntry);
        stochosHBox->addWidget(distLabel);
        stochosHBox->addWidget(m_distEntry);
        stochosHBox->addWidget(m_removeStochosButton);

        m_minEntry->setFixedHeight(20);
        m_maxEntry->setFixedHeight(20);
        m_distEntry->setFixedHeight(20);

        m_mainLayout->addLayout(stochosHBox);
    }
    else if (methodType == 1) {
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(0);
        m_row = new FunctionEntryRow("Value:", stochosIndex, functionReturnENV, true, false, this);
        connect(m_row, &FunctionEntryRow::deleteRequested, this,
                [this](FunctionEntryRow*){ emit deleteRequested(this); });
        connect(m_row, &FunctionEntryRow::textChanged, this,
                [this](FunctionEntryRow*){ emit nodeTextChanged(this); });
        connect(m_row, &FunctionEntryRow::editFocused, this, &Stochos::editFocused);

        m_mainLayout->addWidget(m_row);
    }

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // setFixedHeight(50);
}

QString Stochos::getNodeText() {
    QString nodeText = "";
    if (m_methodType == 0) { // Range
        nodeText += "<Envelope>" + m_minEntry->text()  + "</Envelope>";
        nodeText += "<Envelope>" + m_maxEntry->text()  + "</Envelope>";
        nodeText += "<Envelope>" + m_distEntry->text() + "</Envelope>";
    } else { // Function/Value
        nodeText += "<Envelope>" + m_row->getText() + "</Envelope>";
    }
    return nodeText;
}


void Stochos::onNodeTextChanged() { emit nodeTextChanged(this); }

void Stochos::onRemoveNodeClicked() { emit deleteRequested(this); }

Stochos::~Stochos() {}
