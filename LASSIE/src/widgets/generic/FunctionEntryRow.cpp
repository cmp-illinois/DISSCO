#include "FunctionEntryRow.hpp"
#include "../../dialogs/FunctionGenerator.hpp"

FunctionEntryRow::FunctionEntryRow(const QString& labelText,
                                   int index,
                                   FunctionReturnType fnReturnType,
                                   bool rmVisible,
                                   bool insVisible,
                                   QWidget* parent,
                                   bool fnVisible)
    : QFrame(parent),
      m_index(index),
      m_fnReturnType(fnReturnType)
{
    setFrameShape(QFrame::NoFrame);

    m_hBox    = new QHBoxLayout(this);
    m_label   = new QLabel(labelText);
    m_entry   = new QLineEdit;
    if(fnVisible) { m_fnButton = new QPushButton("fn"); }
    if(rmVisible) { m_rmButton = new QPushButton("rm"); }
    if(insVisible) { m_insButton = new QPushButton("ins"); }

    m_hBox->addWidget(m_label);
    m_hBox->addWidget(m_entry);
    if(fnVisible) { m_hBox->addWidget(m_fnButton); }
    if(rmVisible) { m_hBox->addWidget(m_rmButton); }
    if(insVisible) { m_hBox->addWidget(m_insButton); }

    if(fnVisible) { connect(m_fnButton, &QPushButton::clicked, this, &FunctionEntryRow::onFnClicked); }
    if(rmVisible) { connect(m_rmButton, &QPushButton::clicked, this, &FunctionEntryRow::onRmClicked); }
    if(insVisible) { connect(m_insButton, &QPushButton::clicked, this, &FunctionEntryRow::onInsClicked); }
    connect(m_entry,    &QLineEdit::textChanged,         this, &FunctionEntryRow::onTextChanged);
    connect(m_entry,    &QLineEdit::cursorPositionChanged, this, [this](){ emit editFocused(m_entry); });

    m_entry->setFixedHeight(20);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QString FunctionEntryRow::getText() const {
    return m_entry->text();
}

void FunctionEntryRow::setText(const QString& text) {
    if (m_entry)
        m_entry->setText(text);
}

void FunctionEntryRow::setLabel(const QString& text) {
    if (m_label)
        m_label->setText(text);
}

void FunctionEntryRow::onFnClicked() {
    FunctionGenerator* gen = new FunctionGenerator(nullptr, m_fnReturnType, m_entry->text());
    if (gen) {
        if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty())
            m_entry->setText(gen->getResultString());
        delete gen;
    }
}

void FunctionEntryRow::onRmClicked() {
    emit deleteRequested(this);
}

void FunctionEntryRow::onInsClicked() {
    emit insertRequested(this);
}

void FunctionEntryRow::onTextChanged() {
    emit textChanged(this);
}

FunctionEntryRow::~FunctionEntryRow() {}
