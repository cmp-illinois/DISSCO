#include "SPAFunction.hpp"

#include "../../../widgets/SPAChannel.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

SPAFunction::SPAFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // Method radio
    auto* methodGroup = new QGroupBox(this);
    auto* methodLayout = new QHBoxLayout(methodGroup);
    methodLayout->addWidget(new QLabel(tr("Method:"), methodGroup));
    m_stereo   = new QRadioButton(tr("Stereo"),     methodGroup);
    m_multiPan = new QRadioButton(tr("Multi-Pan"),  methodGroup);
    m_polar    = new QRadioButton(tr("Polar"),      methodGroup);
    m_stereo->setChecked(true);
    methodLayout->addWidget(m_stereo);
    methodLayout->addWidget(m_multiPan);
    methodLayout->addWidget(m_polar);
    layout->addWidget(methodGroup);

    // Apply radio
    auto* applyGroup = new QGroupBox(this);
    auto* applyLayout = new QHBoxLayout(applyGroup);
    applyLayout->addWidget(new QLabel(tr("Apply To:"), applyGroup));
    m_applySound   = new QRadioButton(tr("Sound"),   applyGroup);
    m_applyPartial = new QRadioButton(tr("Partial"), applyGroup);
    m_applySound->setChecked(true);
    applyLayout->addWidget(m_applySound);
    applyLayout->addWidget(m_applyPartial);
    layout->addWidget(applyGroup);

    // Channels scroll area
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

    // Default channels for the initial method
    rebuildChannelsForCurrentMethod();

    auto onMethodOrApply = [this](bool) {
        rebuildChannelsForCurrentMethod();
        emit xmlChanged();
    };
    connect(m_stereo,       &QRadioButton::toggled, this, onMethodOrApply);
    connect(m_multiPan,     &QRadioButton::toggled, this, onMethodOrApply);
    connect(m_polar,        &QRadioButton::toggled, this, onMethodOrApply);
    connect(m_applySound,   &QRadioButton::toggled, this, onMethodOrApply);
    connect(m_applyPartial, &QRadioButton::toggled, this, onMethodOrApply);
}

SPAChannel* SPAFunction::insertChannel(SPAChannel* prevChan) {
    const int insertIdx = prevChan ? m_channels.indexOf(prevChan) + 1 : 0;
    const bool isPartialMode = m_applyPartial->isChecked();

    auto* newChan = new SPAChannel(m_channels.size() + 1, isPartialMode,
                                   m_scrollArea->widget());
    m_channels.insert(insertIdx, newChan);
    m_channelsLayout->insertWidget(insertIdx, newChan);

    const int targetRowCount = m_channels.isEmpty() ? 1 : m_channels.first()->rowCount();
    for (int i = 0; i < targetRowCount; ++i) newChan->addRow(i, true);
    if (newChan->rowCount() == 0) newChan->addRow(0, true);

    connect(newChan, &SPAChannel::textChanged,
            this, [this]() { emit xmlChanged(); });
    connect(newChan, &SPAChannel::insertChannelRequested,
            this, [this](SPAChannel* c) { insertChannel(c); });
    connect(newChan, &SPAChannel::removeChannelRequested,
            this, [this](SPAChannel* c) { removeChannel(c); });
    connect(newChan, &SPAChannel::rowInsertRequested,
            this, [this](int idx) { syncInsertRow(idx); });
    connect(newChan, &SPAChannel::rowRemoveRequested,
            this, [this](int idx) { syncRemoveRow(idx); });

    updateLabels();
    return newChan;
}

void SPAFunction::removeChannel(SPAChannel* chan) {
    if (m_channels.size() <= 1) return;
    const int idx = m_channels.indexOf(chan);
    if (idx == -1) return;
    m_channels.takeAt(idx);
    chan->deleteLater();
    updateLabels();
    emit xmlChanged();
}

void SPAFunction::clearChannels() {
    for (auto* c : m_channels) {
        m_channelsLayout->removeWidget(c);
        delete c;  // immediate -- deleteLater leaves ghosts during rebuild
    }
    m_channels.clear();
}

void SPAFunction::rebuildChannelsForCurrentMethod() {
    clearChannels();
    if (m_stereo->isChecked()) {
        auto* c = insertChannel(nullptr);
        c->hideTitle();
        c->hideButtons();
    } else if (m_multiPan->isChecked()) {
        insertChannel(nullptr);
    } else if (m_polar->isChecked()) {
        auto* theta = insertChannel(nullptr);
        theta->setTitle(QStringLiteral("Theta"));
        theta->hideButtons();
        auto* rad = insertChannel(theta);
        rad->setTitle(QStringLiteral("Radius"));
        rad->hideButtons();
    }
}

void SPAFunction::syncInsertRow(int index) {
    for (auto* c : m_channels) c->addRow(index + 1, true);
    emit xmlChanged();
}

void SPAFunction::syncRemoveRow(int index) {
    for (auto* c : m_channels) c->removeRow(index);
    emit xmlChanged();
}

void SPAFunction::updateLabels() {
    if (m_polar->isChecked()) return;  // labels are fixed (Theta/Radius)
    for (int i = 0; i < m_channels.size(); ++i)
        m_channels[i]->setTitle(QStringLiteral("Channel %1:").arg(i + 1));
}

QString SPAFunction::currentMethod() const {
    if (m_multiPan->isChecked()) return QStringLiteral("MULTI_PAN");
    if (m_polar->isChecked())    return QStringLiteral("POLAR");
    return QStringLiteral("STEREO");
}

QString SPAFunction::currentApply() const {
    return m_applyPartial->isChecked() ? QStringLiteral("PARTIAL")
                                        : QStringLiteral("SOUND");
}

QString SPAFunction::buildXMLString() const {
    QString out = QStringLiteral("<Fun><Name>SPA</Name><Method>")
                + currentMethod()
                + QStringLiteral("</Method><Apply>")
                + currentApply()
                + QStringLiteral("</Apply><Channels>");
    for (auto* c : m_channels) {
        out += QStringLiteral("<Partials>");
        out += c->getXml();
        out += QStringLiteral("</Partials>");
    }
    out += QStringLiteral("</Channels></Fun>");
    return out;
}

void SPAFunction::populateFromXML(QXmlStreamReader& reader) {
    const QString method = nextChildInner(reader);
    const QString apply  = nextChildInner(reader);

    // Block radio signals so setChecked doesn't trigger another rebuild
    // for each toggle; we'll rebuild once explicitly below.
    {
        QSignalBlocker b1(m_stereo);
        QSignalBlocker b2(m_multiPan);
        QSignalBlocker b3(m_polar);
        QSignalBlocker b4(m_applySound);
        QSignalBlocker b5(m_applyPartial);
        if      (method == QStringLiteral("MULTI_PAN")) m_multiPan->setChecked(true);
        else if (method == QStringLiteral("POLAR"))     m_polar->setChecked(true);
        else                                             m_stereo->setChecked(true);
        if (apply == QStringLiteral("PARTIAL")) m_applyPartial->setChecked(true);
        else                                     m_applySound->setChecked(true);
    }
    rebuildChannelsForCurrentMethod();

    if (reader.readNextStartElement()) { // <Channels>
        int chanIdx = 0;
        while (reader.readNextStartElement()) { // <Partials>
            while (chanIdx >= m_channels.size())
                insertChannel(m_channels.isEmpty() ? nullptr : m_channels.last());
            auto* chan = m_channels[chanIdx];
            int rowIdx = 0;
            while (reader.readNextStartElement()) { // <P>
                while (rowIdx >= chan->rowCount())
                    chan->addRow(chan->rowCount(), true);
                chan->setRowText(rowIdx, readInner(reader));
                ++rowIdx;
            }
            ++chanIdx;
        }
    }
}

void SPAFunction::reset() {
    QSignalBlocker b1(m_stereo);
    QSignalBlocker b2(m_multiPan);
    QSignalBlocker b3(m_polar);
    QSignalBlocker b4(m_applySound);
    QSignalBlocker b5(m_applyPartial);
    m_stereo->setChecked(true);
    m_applySound->setChecked(true);
    rebuildChannelsForCurrentMethod();
}
