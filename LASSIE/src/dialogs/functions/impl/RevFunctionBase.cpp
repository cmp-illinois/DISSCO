#include "RevFunctionBase.hpp"

#include "../../../widgets/REVChannel.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

RevFunctionBase::RevFunctionBase(int mode, QWidget* parent)
    : FunctionWidget(parent),
      m_mode(mode)
{
    auto* layout = new QVBoxLayout(this);

    auto* applyGroup = new QGroupBox(this);
    auto* applyLayout = new QHBoxLayout(applyGroup);
    applyLayout->addWidget(new QLabel(tr("Apply To:"), applyGroup));
    m_applySound   = new QRadioButton(tr("Sound"),   applyGroup);
    m_applyPartial = new QRadioButton(tr("Partial"), applyGroup);
    m_applySound->setChecked(true);
    applyLayout->addWidget(m_applySound);
    applyLayout->addWidget(m_applyPartial);
    layout->addWidget(applyGroup);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setMinimumSize(400, 200);
    auto* scrollContents = new QWidget;
    m_channelsLayout = new QVBoxLayout(scrollContents);
    m_channelsLayout->setSpacing(5);
    m_channelsLayout->setContentsMargins(5, 5, 5, 5);
    m_channelsLayout->addStretch(1);
    m_scrollArea->setWidget(scrollContents);
    layout->addWidget(m_scrollArea);

    rebuildDefaults();

    auto onApplyToggled = [this](bool) {
        rebuildDefaults();
        emit xmlChanged();
    };
    connect(m_applySound,   &QRadioButton::toggled, this, onApplyToggled);
    connect(m_applyPartial, &QRadioButton::toggled, this, onApplyToggled);
}

REVChannel* RevFunctionBase::insertChannel(REVChannel* prevChan) {
    const int insertIdx = prevChan ? m_channels.indexOf(prevChan) + 1 : 0;
    auto* newChan = new REVChannel(m_channels.size() + 1, m_mode,
                                   m_scrollArea->widget());
    m_channels.insert(insertIdx, newChan);
    // Insert before the trailing stretch so new channels stack from the top.
    m_channelsLayout->insertWidget(insertIdx, newChan);

    connect(newChan, &REVChannel::textChanged,
            this, [this]() { emit xmlChanged(); });
    connect(newChan, &REVChannel::insertChannelRequested,
            this, [this](REVChannel* c) { insertChannel(c); });
    connect(newChan, &REVChannel::removeChannelRequested,
            this, [this](REVChannel* c) { removeChannel(c); });

    updateLabels();
    return newChan;
}

void RevFunctionBase::removeChannel(REVChannel* chan) {
    if (m_channels.size() <= 1) return;
    const int idx = m_channels.indexOf(chan);
    if (idx == -1) return;
    m_channels.takeAt(idx);
    chan->deleteLater();
    updateLabels();
    emit xmlChanged();
}

void RevFunctionBase::clearChannels() {
    for (auto* c : m_channels) {
        m_channelsLayout->removeWidget(c);
        delete c;  // immediate to avoid ghosts during rebuild
    }
    m_channels.clear();
}

void RevFunctionBase::rebuildDefaults() {
    clearChannels();
    auto* first = insertChannel(nullptr);
    if (m_applySound->isChecked()) {
        first->setTitle(QStringLiteral("Sound"));
        first->hideButtons();
    }
}

void RevFunctionBase::updateLabels() {
    if (m_applySound->isChecked()) return;  // first channel keeps "Sound"
    for (int i = 0; i < m_channels.size(); ++i)
        m_channels[i]->setTitle(QStringLiteral("Partial %1:").arg(i + 1));
}

QString RevFunctionBase::currentApply() const {
    return m_applyPartial->isChecked() ? QStringLiteral("PARTIAL")
                                        : QStringLiteral("SOUND");
}

QString RevFunctionBase::buildXMLString() const {
    QString out = QStringLiteral("<Fun><Name>") + xmlName()
                + QStringLiteral("</Name><Apply>") + currentApply()
                + QStringLiteral("</Apply>");

    auto wrap = [&](const QString& tag, auto getter) {
        out += QStringLiteral("<") + tag + QStringLiteral(">");
        for (auto* c : m_channels) out += (c->*getter)();
        out += QStringLiteral("</") + tag + QStringLiteral(">");
    };

    switch (m_mode) {
        case 0:  // Simple
            wrap(QStringLiteral("Sizes"), &REVChannel::getXmlRoom);
            break;
        case 1:  // Medium
            wrap(QStringLiteral("Percents"),  &REVChannel::getXmlReverb);
            wrap(QStringLiteral("Spreads"),   &REVChannel::getXmlHillow);
            wrap(QStringLiteral("AllPasses"), &REVChannel::getXmlAllGain);
            wrap(QStringLiteral("Delays"),    &REVChannel::getXmlDelay);
            break;
        case 2:  // Advanced
            wrap(QStringLiteral("Percents"),      &REVChannel::getXmlReverb);
            wrap(QStringLiteral("CombGainLists"), &REVChannel::getXmlCombGain);
            wrap(QStringLiteral("LPGainLists"),   &REVChannel::getXmlLPGain);
            wrap(QStringLiteral("AllPasses"),     &REVChannel::getXmlAllGain);
            wrap(QStringLiteral("Delays"),        &REVChannel::getXmlDelay);
            break;
    }
    out += QStringLiteral("</Fun>");
    return out;
}

void RevFunctionBase::populateFromXML(QXmlStreamReader& reader) {
    const QString applyMethod = nextChildInner(reader);
    {
        QSignalBlocker bs(m_applySound);
        QSignalBlocker bp(m_applyPartial);
        if (applyMethod == QStringLiteral("PARTIAL")) {
            m_applyPartial->setChecked(true);
            m_applySound->setChecked(false);
        } else {
            m_applySound->setChecked(true);
            m_applyPartial->setChecked(false);
        }
    }
    clearChannels();

    auto readList = [&](QStringList& out) {
        if (!reader.readNextStartElement()) return;
        while (reader.readNextStartElement())
            out.append(readInner(reader));
    };

    if (m_mode == 0) {
        QStringList sizes;
        readList(sizes);
        for (const QString& s : sizes) {
            auto* chan = insertChannel(m_channels.isEmpty() ? nullptr : m_channels.last());
            chan->setRoomSizeText(s);
        }
    } else if (m_mode == 1) {
        QStringList percents, spreads, allpasses, delays;
        readList(percents); readList(spreads); readList(allpasses); readList(delays);
        for (int i = 0; i < percents.size(); ++i) {
            auto* chan = insertChannel(m_channels.isEmpty() ? nullptr : m_channels.last());
            chan->setReverbText(percents[i]);
            if (i < spreads.size())   chan->setHillowText(spreads[i]);
            if (i < allpasses.size()) chan->setAllGainText(allpasses[i]);
            if (i < delays.size())    chan->setDelayText(delays[i]);
        }
    } else {  // Advanced
        QStringList percents, combs, lps, allpasses, delays;
        readList(percents); readList(combs); readList(lps); readList(allpasses); readList(delays);
        for (int i = 0; i < percents.size(); ++i) {
            auto* chan = insertChannel(m_channels.isEmpty() ? nullptr : m_channels.last());
            chan->setReverbText(percents[i]);
            if (i < combs.size())     chan->setCombGainText(combs[i]);
            if (i < lps.size())       chan->setLPGainText(lps[i]);
            if (i < allpasses.size()) chan->setAllGainText(allpasses[i]);
            if (i < delays.size())    chan->setDelayText(delays[i]);
        }
    }

    // Restore SOUND-mode presentation if applicable.
    if (applyMethod != QStringLiteral("PARTIAL") && !m_channels.isEmpty()) {
        m_channels.first()->setTitle(QStringLiteral("Sound"));
        m_channels.first()->hideButtons();
    }
}

void RevFunctionBase::reset() {
    QSignalBlocker bs(m_applySound);
    QSignalBlocker bp(m_applyPartial);
    m_applySound->setChecked(true);
    m_applyPartial->setChecked(false);
    rebuildDefaults();
}
