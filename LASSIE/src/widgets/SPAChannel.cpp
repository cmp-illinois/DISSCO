#include "SPAChannel.hpp"

using enum FunctionReturnType;

SPAChannel::SPAChannel(int id, bool isPartialMode, QWidget* parent) 
    : QFrame(parent), m_isPartialMode(isPartialMode) {
        
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Sunken);

    auto* layout = new QVBoxLayout(this);
    m_titleLabel = new QLabel(QString("Channel %1").arg(id));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    m_insBtn = new QPushButton("Insert Channel After");
    m_rmBtn  = new QPushButton("Remove This Channel");
        
    m_rowsContainer = new QWidget();
    m_rowsLayout = new QVBoxLayout(m_rowsContainer);
    m_rowsLayout->setSpacing(0);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->addStretch();

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_insBtn);
    layout->addWidget(m_rmBtn);
    layout->addWidget(m_rowsContainer);

    connect(m_insBtn, &QPushButton::clicked, this, [this](){ emit insertChannelRequested(this); });
    connect(m_rmBtn,  &QPushButton::clicked, this, [this](){ emit removeChannelRequested(this); });
}

void SPAChannel::addRow(int index, bool silent) {
    QString label = m_isPartialMode ? QString("Partial %1:").arg(m_rows.size() + 1) : "Envelope:";
    auto* row = new FunctionEntryRow(label, index, functionReturnENV, m_isPartialMode, m_isPartialMode, this);

    m_rows.insert(index, row);
    m_rowsLayout->insertWidget(index, row);
    if (m_isPartialMode) updateLabels();

    connect(row, &FunctionEntryRow::textChanged, this, &SPAChannel::textChanged);
    if (m_isPartialMode) {
        connect(row, &FunctionEntryRow::deleteRequested, this, [this, row](){
            emit rowRemoveRequested(m_rows.indexOf(row));
        });
        connect(row, &FunctionEntryRow::insertRequested, this, [this, row](){
            emit rowInsertRequested(m_rows.indexOf(row));
        });
    }
    if (!silent) emit rowInsertRequested(index);
}

void SPAChannel::removeRow(int index) {
    if (m_rows.size() <= 1) return;
    auto* row = m_rows.takeAt(index);
    row->deleteLater();
    updateLabels();
}

QString SPAChannel::getXml() {
    QString xml;
    for (auto* row : m_rows) xml += "<P>" + row->getText() + "</P>";
    return xml;
}

void SPAChannel::updateLabels() {
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i]->setLabel(m_isPartialMode ? QString("Partial %1:").arg(i+1) : "Envelope:");
    }
}

void SPAChannel::setTitle(const QString& text) {
    if (m_titleLabel) m_titleLabel->setText(text);
}

void SPAChannel::setRowText(int index, const QString& text) {
    if (index >= 0 && index < m_rows.size())
        m_rows[index]->setText(text);
}

void SPAChannel::hideButtons() {
    m_insBtn->hide();
    m_rmBtn->hide();
}

void SPAChannel::hideTitle() {
    m_titleLabel->hide();
}
