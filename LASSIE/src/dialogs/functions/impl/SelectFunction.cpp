#include "SelectFunction.hpp"

#include "../../../widgets/Select.hpp"
#include "../../../widgets/generic/FunctionEntryRow.hpp"
#include "../../FunctionGenerator.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

SelectFunction::SelectFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // Top: Choice Index
    m_indexRow = new FunctionEntryRow(tr("Choice Index:"), 0,
                                      FunctionReturnType::functionReturnInt,
                                      false, false, this);
    layout->addWidget(m_indexRow);
    connect(m_indexRow, &FunctionEntryRow::textChanged,
            this, [this]() { emit xmlChanged(); });

    // Middle: scrollable list of Select rows
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setMinimumSize(400, 200);
    auto* scrollContents = new QWidget;
    m_nodesLayout = new QVBoxLayout(scrollContents);
    m_nodesLayout->addStretch(1);
    m_scrollArea->setWidget(scrollContents);
    layout->addWidget(m_scrollArea);

    // Bottom: Add Node + (focused-row) Insert Function
    auto* buttonRow = new QHBoxLayout;
    auto* addButton = new QPushButton(tr("Add New Node"), this);
    m_insertFnButton = new QPushButton(tr("Insert Function"), this);
    buttonRow->addWidget(addButton);
    buttonRow->addWidget(m_insertFnButton);
    layout->addLayout(buttonRow);

    connect(addButton, &QPushButton::clicked, this, [this]() { addNode(); });
    connect(m_insertFnButton, &QPushButton::clicked, this, [this]() {
        if (!m_lastFocusedEdit) return;
        FunctionGenerator gen(this, FunctionReturnType::functionReturnMakeListFun,
                              m_lastFocusedEdit->text());
        if (gen.exec() == QDialog::Accepted && !gen.getResultString().isEmpty())
            m_lastFocusedEdit->setText(gen.getResultString());
    });
}

void SelectFunction::addNode(const QString& initialText) {
    const int insertAt = m_nodesLayout->count() - 1;  // before the trailing stretch
    auto* node = new Select(insertAt, m_scrollArea->widget());

    connect(node, &Select::deleteRequested,
            this, [this](Select* n) { removeNode(n); });
    connect(node, &Select::nodeTextChanged,
            this, [this](Select*) { emit xmlChanged(); });
    connect(node, &Select::editFocused,
            this, [this](QLineEdit* edit) { m_lastFocusedEdit = edit; });

    if (!initialText.isEmpty()) node->setValText(initialText);
    m_nodesLayout->insertWidget(insertAt, node);
    emit xmlChanged();
}

void SelectFunction::removeNode(Select* node) {
    if (!node) return;
    if (m_lastFocusedEdit && node->isAncestorOf(m_lastFocusedEdit))
        m_lastFocusedEdit = nullptr;
    m_nodesLayout->removeWidget(node);
    node->deleteLater();
    reindexNodes();
    emit xmlChanged();
}

void SelectFunction::reindexNodes() {
    for (int i = 0; i < m_nodesLayout->count() - 1; ++i) {
        if (auto* item = m_nodesLayout->itemAt(i))
            if (auto* n = qobject_cast<Select*>(item->widget()))
                n->setSelectIndex(i);
    }
}

QString SelectFunction::joinedNodesText() const {
    QStringList parts;
    for (int i = 0; i < m_nodesLayout->count() - 1; ++i) {
        if (auto* item = m_nodesLayout->itemAt(i))
            if (auto* n = qobject_cast<Select*>(item->widget()))
                parts << n->getNodeText();
    }
    return parts.join(QStringLiteral(", "));
}

// Split on commas that aren't inside a <Fun>...</Fun> block. Matches the
// legacy parser's behavior so nested function strings round-trip cleanly.
QStringList SelectFunction::splitTopLevelCommas(const QString& s) {
    QStringList out;
    int start = 0;
    int depth = 0;
    int i = 0;
    while (i < s.size()) {
        if (s.mid(i, 5) == QStringLiteral("<Fun>"))  { ++depth; i += 5; continue; }
        if (s.mid(i, 6) == QStringLiteral("</Fun>")) { --depth; i += 6; continue; }
        if (depth == 0 && s[i] == QChar(',')) {
            out << s.mid(start, i - start).trimmed();
            start = i + 1;
        }
        ++i;
    }
    if (start < s.size()) out << s.mid(start).trimmed();
    return out;
}

QString SelectFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>Select</Name><List>")
         + joinedNodesText()
         + QStringLiteral("</List><Index>")
         + m_indexRow->getText()
         + QStringLiteral("</Index></Fun>");
}

void SelectFunction::populateFromXML(QXmlStreamReader& reader) {
    // <List> contains either text or nested elements; readInner gets us
    // a single string with nested <Fun>...</Fun> blocks intact.
    if (!reader.readNextStartElement()) return;
    const QString listContent = readInner(reader);
    for (const QString& item : splitTopLevelCommas(listContent))
        if (!item.isEmpty()) addNode(item);

    m_indexRow->setText(nextChildInner(reader));
}

void SelectFunction::reset() {
    while (m_nodesLayout->count() > 1) {
        if (auto* item = m_nodesLayout->itemAt(0)) {
            if (auto* w = item->widget()) {
                m_nodesLayout->removeWidget(w);
                w->deleteLater();
            }
        }
    }
    m_lastFocusedEdit = nullptr;
    m_indexRow->setText(QString());
}
