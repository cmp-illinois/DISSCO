#include "StochosFunction.hpp"

#include "../../../widgets/Stochos.hpp"
#include "../../FunctionGenerator.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>

StochosFunction::StochosFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // Method radio
    auto* methodGroup = new QGroupBox(this);
    auto* methodLayout = new QHBoxLayout(methodGroup);
    methodLayout->addWidget(new QLabel(tr("Method:"), methodGroup));
    m_methodRange     = new QRadioButton(tr("Range Distribution"), methodGroup);
    m_methodFunctions = new QRadioButton(tr("Functions"),          methodGroup);
    m_methodRange->setChecked(true);
    methodLayout->addWidget(m_methodRange);
    methodLayout->addWidget(m_methodFunctions);
    layout->addWidget(methodGroup);

    // Offset
    auto* offsetRow = new QHBoxLayout;
    offsetRow->addWidget(new QLabel(tr("Offset:"), this));
    m_offsetEdit = new QLineEdit(this);
    offsetRow->addWidget(m_offsetEdit);
    layout->addLayout(offsetRow);

    // Scroll area for nodes
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setMinimumSize(400, 200);
    auto* scrollContents = new QWidget;
    m_nodesLayout = new QVBoxLayout(scrollContents);
    m_nodesLayout->addStretch(1);
    m_scrollArea->setWidget(scrollContents);
    layout->addWidget(m_scrollArea);

    // Bottom: Add + Insert Function buttons
    auto* buttonRow = new QHBoxLayout;
    auto* addButton = new QPushButton(tr("Add New Node"), this);
    m_insertFnButton = new QPushButton(tr("Insert Function"), this);
    buttonRow->addWidget(addButton);
    buttonRow->addWidget(m_insertFnButton);
    layout->addLayout(buttonRow);

    auto forward = [this]() { emit xmlChanged(); };
    connect(m_methodRange,     &QRadioButton::toggled,
            this, [this](bool) { clearNodes(); emit xmlChanged(); });
    connect(m_methodFunctions, &QRadioButton::toggled,
            this, [this](bool) { clearNodes(); emit xmlChanged(); });
    connect(m_offsetEdit, &QLineEdit::textChanged, this, forward);
    connect(addButton, &QPushButton::clicked, this, [this]() { addNode(); });
    connect(m_insertFnButton, &QPushButton::clicked, this, [this]() {
        if (!m_lastFocusedEdit) return;
        FunctionGenerator gen(this, FunctionReturnType::functionReturnENV,
                              m_lastFocusedEdit->text());
        if (gen.exec() == QDialog::Accepted && !gen.getResultString().isEmpty())
            m_lastFocusedEdit->setText(gen.getResultString());
    });
}

bool StochosFunction::isRangeMode() const {
    return m_methodRange->isChecked();
}

void StochosFunction::addNode() {
    const int insertAt = m_nodesLayout->count() - 1;
    const int methodType = isRangeMode() ? 0 : 1;
    auto* node = new Stochos(methodType, insertAt, m_scrollArea->widget());

    connect(node, &Stochos::deleteRequested,
            this, [this](Stochos* n) { removeNode(n); });
    connect(node, &Stochos::nodeTextChanged,
            this, [this](Stochos*) { emit xmlChanged(); });
    connect(node, &Stochos::editFocused,
            this, [this](QLineEdit* edit) { m_lastFocusedEdit = edit; });

    m_nodesLayout->insertWidget(insertAt, node);
    emit xmlChanged();
}

void StochosFunction::removeNode(Stochos* node) {
    if (!node) return;
    if (m_lastFocusedEdit && node->isAncestorOf(m_lastFocusedEdit))
        m_lastFocusedEdit = nullptr;
    m_nodesLayout->removeWidget(node);
    node->deleteLater();
    reindexNodes();
    emit xmlChanged();
}

void StochosFunction::clearNodes() {
    while (m_nodesLayout->count() > 1) {
        if (auto* item = m_nodesLayout->itemAt(0)) {
            if (auto* w = item->widget()) {
                m_nodesLayout->removeWidget(w);
                w->deleteLater();
            }
        }
    }
    m_lastFocusedEdit = nullptr;
}

void StochosFunction::reindexNodes() {
    for (int i = 0; i < m_nodesLayout->count() - 1; ++i) {
        if (auto* item = m_nodesLayout->itemAt(i))
            if (auto* n = qobject_cast<Stochos*>(item->widget()))
                n->setStochosIndex(i);
    }
}

QString StochosFunction::joinedNodesText() const {
    QString out;
    for (int i = 0; i < m_nodesLayout->count() - 1; ++i) {
        if (auto* item = m_nodesLayout->itemAt(i))
            if (auto* n = qobject_cast<Stochos*>(item->widget()))
                out += n->getNodeText();
    }
    return out;
}

QString StochosFunction::buildXMLString() const {
    const QString method = isRangeMode()
        ? QStringLiteral("RANGE_DISTRIB")
        : QStringLiteral("FUNCTIONS");
    return QStringLiteral("<Fun><Name>Stochos</Name><Method>")
         + method
         + QStringLiteral("</Method><Envelopes>")
         + joinedNodesText()
         + QStringLiteral("</Envelopes><Offset>")
         + m_offsetEdit->text()
         + QStringLiteral("</Offset></Fun>");
}

void StochosFunction::populateFromXML(QXmlStreamReader& reader) {
    const QString method = nextChildInner(reader);
    const bool wantRange = (method != QStringLiteral("FUNCTIONS"));
    // Avoid the togggled-handler's clearNodes (no nodes exist yet
    // anyway) firing twice.
    QSignalBlocker br(m_methodRange);
    QSignalBlocker bf(m_methodFunctions);
    m_methodRange->setChecked(wantRange);
    m_methodFunctions->setChecked(!wantRange);

    // <Envelopes> wrapper, then a sequence of <Envelope> children.
    if (reader.readNextStartElement()) {
        while (reader.readNextStartElement()) {
            addNode();
            const int last = m_nodesLayout->count() - 2;
            auto* item = m_nodesLayout->itemAt(last);
            auto* node = item ? qobject_cast<Stochos*>(item->widget()) : nullptr;
            if (!node) { reader.skipCurrentElement(); break; }
            if (wantRange) {
                node->setMinText(readInner(reader));
                if (reader.readNextStartElement()) node->setMaxText(readInner(reader));
                if (reader.readNextStartElement()) node->setDistText(readInner(reader));
            } else {
                node->setValText(readInner(reader));
            }
        }
    }

    m_offsetEdit->setText(nextChildInner(reader));
}

void StochosFunction::reset() {
    clearNodes();
    QSignalBlocker br(m_methodRange);
    QSignalBlocker bf(m_methodFunctions);
    m_methodRange->setChecked(true);
    m_methodFunctions->setChecked(false);
    m_offsetEdit->clear();
}
