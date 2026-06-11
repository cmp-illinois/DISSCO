#include "MakeEnvelopeFunction.hpp"

#include "../../../widgets/MakeEnvelopeRow.hpp"
#include "../../../widgets/generic/FunctionEntryRow.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

MakeEnvelopeFunction::MakeEnvelopeFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_scaleRow = new FunctionEntryRow(tr("Scaling Factor:"), 0,
                                      FunctionReturnType::functionReturnFloat,
                                      false, false, this);
    m_scaleRow->setText(QStringLiteral("1.0"));
    layout->addWidget(m_scaleRow);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setMinimumSize(400, 200);
    auto* scrollContents = new QWidget;
    m_rowsLayout = new QVBoxLayout(scrollContents);
    m_rowsLayout->setSpacing(5);
    m_rowsLayout->setContentsMargins(5, 5, 5, 5);
    m_rowsLayout->addStretch(1);
    m_scrollArea->setWidget(scrollContents);
    layout->addWidget(m_scrollArea);

    auto* addRow = new QHBoxLayout;
    auto* addButton = new QPushButton(tr("Add Point"), this);
    addRow->addWidget(addButton);
    addRow->addStretch(1);
    layout->addLayout(addRow);

    connect(m_scaleRow, &FunctionEntryRow::textChanged,
            this, [this]() { emit xmlChanged(); });
    connect(addButton, &QPushButton::clicked, this,
            [this]() { insertRow(m_rows.isEmpty() ? nullptr : m_rows.last()); });

    // Two default points -- mirrors the legacy initial state.
    insertRow(nullptr);
    insertRow(m_rows.last());
}

MakeEnvelopeRow* MakeEnvelopeFunction::insertRow(MakeEnvelopeRow* prevRow) {
    const int index = prevRow ? m_rows.indexOf(prevRow) + 1 : 0;
    auto* row = new MakeEnvelopeRow(index, m_scrollArea->widget());
    m_rows.insert(index, row);
    m_rowsLayout->insertWidget(index, row);
    updateRowLabels();

    connect(row, &MakeEnvelopeRow::textChanged,
            this, [this](MakeEnvelopeRow*) { emit xmlChanged(); });
    connect(row, &MakeEnvelopeRow::deleteRequested,
            this, [this](MakeEnvelopeRow* r) { removeRow(r); });
    connect(row, &MakeEnvelopeRow::insertRequested,
            this, [this](MakeEnvelopeRow* r) { insertRow(r); });

    emit xmlChanged();
    return row;
}

void MakeEnvelopeFunction::removeRow(MakeEnvelopeRow* row) {
    if (m_rows.size() <= 2) return;  // legacy invariant: at least two points
    const int index = m_rows.indexOf(row);
    if (index < 0) return;
    m_rows.removeAt(index);
    m_rowsLayout->removeWidget(row);
    row->deleteLater();
    updateRowLabels();
    emit xmlChanged();
}

void MakeEnvelopeFunction::clearRows() {
    for (auto* row : m_rows) {
        m_rowsLayout->removeWidget(row);
        delete row;  // immediate, to avoid ghosts during rebuild
    }
    m_rows.clear();
}

void MakeEnvelopeFunction::updateRowLabels() {
    for (int i = 0; i < m_rows.size(); ++i)
        m_rows[i]->setLabel(QStringLiteral("Point %1:").arg(i + 1));
}

QString MakeEnvelopeFunction::buildXMLString() const {
    QString xs, ys, types, pros;
    for (int i = 0; i < m_rows.size(); ++i) {
        xs += QStringLiteral("<X>") + m_rows[i]->getX() + QStringLiteral("</X>");
        ys += QStringLiteral("<Y>") + m_rows[i]->getY() + QStringLiteral("</Y>");
        // Types/Pros describe segments between adjacent points, so there
        // are N-1 of them.
        if (i < m_rows.size() - 1) {
            types += QStringLiteral("<T>") + m_rows[i]->getType() + QStringLiteral("</T>");
            pros  += QStringLiteral("<P>") + m_rows[i]->getPro()  + QStringLiteral("</P>");
        }
    }
    return QStringLiteral("<Fun><Name>MakeEnvelope</Name><Xs>")
         + xs
         + QStringLiteral("</Xs><Ys>")    + ys
         + QStringLiteral("</Ys><Types>") + types
         + QStringLiteral("</Types><Pros>") + pros
         + QStringLiteral("</Pros><Scale>") + m_scaleRow->getText()
         + QStringLiteral("</Scale></Fun>");
}

void MakeEnvelopeFunction::populateFromXML(QXmlStreamReader& reader) {
    // Read each wrapper into a flat list, then zip them into rows.
    QStringList xs, ys, types, pros;
    auto readList = [&](QStringList& out) {
        if (!reader.readNextStartElement()) return;
        while (reader.readNextStartElement())
            out.append(readInner(reader));
    };
    readList(xs);
    readList(ys);
    readList(types);
    readList(pros);

    clearRows();
    const int n = qMax(xs.size(), ys.size());
    for (int i = 0; i < n; ++i) {
        auto* row = insertRow(m_rows.isEmpty() ? nullptr : m_rows.last());
        if (i < xs.size())    row->setX(xs[i]);
        if (i < ys.size())    row->setY(ys[i]);
        if (i < types.size()) row->setType(types[i]);
        if (i < pros.size())  row->setPro(pros[i]);
    }

    m_scaleRow->setText(nextChildInner(reader));
}

void MakeEnvelopeFunction::reset() {
    clearRows();
    insertRow(nullptr);
    insertRow(m_rows.last());
    m_scaleRow->setText(QStringLiteral("1.0"));
}
