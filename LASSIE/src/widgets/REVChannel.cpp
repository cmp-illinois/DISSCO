#include "REVChannel.hpp"

using enum FunctionReturnType;

REVChannel::REVChannel(int id, int revMode, QWidget* parent) 
    : QFrame(parent), m_revMode(revMode) {
        
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Sunken);

    auto* layout = new QVBoxLayout(this);
    m_titleLabel = new QLabel(QString("Partial %1").arg(id));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    m_insBtn = new QPushButton("Insert Partial After");
    m_rmBtn  = new QPushButton("Remove This Partial");
        
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

    if (revMode == 0) { // simple
        room_size = new FunctionEntryRow("Room Size: ", 0, functionReturnFloat, false, false, this);
        connect(room_size, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(0, room_size);

    } else if (revMode == 1) { // medium
        reverb = new FunctionEntryRow("Reverb Percentage: ", 0, functionReturnENV, false, false, this);
        connect(reverb, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(0, reverb);

        hillow = new FunctionEntryRow("Hillow Spread: ", 0, functionReturnFloat, false, false, this);
        connect(hillow, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(1, hillow);

        all_gain = new FunctionEntryRow("Gain All Pass: ", 0, functionReturnFloat, false, false, this);
        connect(all_gain, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(2, all_gain);

        delay = new FunctionEntryRow("Delay: ", 0, functionReturnFloat, false, false, this);
        connect(delay, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(3, delay);

    } else if (revMode == 2) { // advanced
        reverb = new FunctionEntryRow("Reverb Percentage: ", 0, functionReturnENV, false, false, this);
        connect(reverb, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(0, reverb);

        comb_gain = new FunctionEntryRow("Comb Gain List: ", 0, functionReturnList, false, false, this);
        connect(comb_gain, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(1, comb_gain);

        lp_gain = new FunctionEntryRow("LP Gain List: ", 0, functionReturnList, false, false, this);
        connect(lp_gain, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(2, lp_gain);

        all_gain = new FunctionEntryRow("Gain All Pass: ", 0, functionReturnFloat, false, false, this);
        connect(all_gain, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(3, all_gain);

        delay = new FunctionEntryRow("Delay: ", 0, functionReturnFloat, false, false, this);
        connect(delay, &FunctionEntryRow::textChanged, this, &REVChannel::textChanged);
        m_rowsLayout->insertWidget(4, delay);

    }
}

QString REVChannel::getXmlRoom() {
    QString xml;
    if (room_size) xml += "<Size>" + room_size->getText() + "</Size>";
    return xml;
}

QString REVChannel::getXmlReverb() {
    QString xml;
    if (reverb) xml += "<Percent>" + reverb->getText() + "</Percent>";
    return xml;
}

QString REVChannel::getXmlHillow() {
    QString xml;
    if (hillow) xml += "<Spread>" + hillow->getText() + "</Spread>";
    return xml;
}

QString REVChannel::getXmlAllGain() {
    QString xml;
    if (all_gain) xml += "<AllPass>" + all_gain->getText() + "</AllPass>";
    return xml;
}

QString REVChannel::getXmlDelay() {
    QString xml;
    if (delay) xml += "<Delay>" + delay->getText() + "</Delay>";
    return xml;
}

QString REVChannel::getXmlCombGain() {
    QString xml;
    if (comb_gain) xml += "<CombGainList>" + comb_gain->getText() + "</CombGainList>";
    return xml;
}

QString REVChannel::getXmlLPGain() {
    QString xml;
    if (lp_gain) xml += "<LPGainList>" + lp_gain->getText() + "</LPGainList>";
    return xml;
}

void REVChannel::setTitle(const QString& text) { 
    if (m_titleLabel) m_titleLabel->setText(text); 
}

void REVChannel::hideButtons() {
    m_insBtn->hide();
    m_rmBtn->hide();
}

void REVChannel::setRoomSizeText(const QString& text) { if (room_size) room_size->setText(text); }
void REVChannel::setReverbText(const QString& text)   { if (reverb)    reverb->setText(text); }
void REVChannel::setHillowText(const QString& text)   { if (hillow)    hillow->setText(text); }
void REVChannel::setAllGainText(const QString& text)  { if (all_gain)  all_gain->setText(text); }
void REVChannel::setDelayText(const QString& text)    { if (delay)     delay->setText(text); }
void REVChannel::setCombGainText(const QString& text) { if (comb_gain) comb_gain->setText(text); }
void REVChannel::setLPGainText(const QString& text)   { if (lp_gain)   lp_gain->setText(text); }