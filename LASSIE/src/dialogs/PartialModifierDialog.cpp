#include "PartialModifierDialog.hpp"

#include "FunctionGenerator.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>

PartialModifierDialog::PartialModifierDialog(QWidget* parent,
                                             int spectrumPartialCount,
                                             const QString& originalString)
    : QDialog(parent)
{
    setWindowTitle(tr("Customize Partials"));
    setModal(true);
    resize(900, 700);

    auto* mainLayout = new QVBoxLayout(this);
    auto* explanation = new QLabel(
        tr("Each partial is stored as probability, magnitude, width, and rate ENV values. "
           "Use Insert Function to build an envelope; N/A leaves an unused value empty."),
        this);
    explanation->setWordWrap(true);
    mainLayout->addWidget(explanation);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    QString parseWarning;
    QVector<PartialModifierFormat::Values> values =
        PartialModifierFormat::parse(originalString, &parseWarning);
    const int requestedRows = std::max(1, spectrumPartialCount);
    const int rowCount = std::max(requestedRows, static_cast<int>(values.size()));
    values.resize(rowCount);

    if (parseWarning.isEmpty()) {
        m_statusLabel->setText(
            tr("Editing %1 partial(s), based on the largest Spectrum in this project.")
                .arg(rowCount));
    } else {
        m_statusLabel->setText(parseWarning + tr(" Cancel preserves the original value."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #b06000;"));
    }

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto* rowsWidget = new QWidget(scrollArea);
    m_rowsLayout = new QVBoxLayout(rowsWidget);
    for (int i = 0; i < rowCount; ++i)
        addPartialRow(i, values.at(i));
    m_rowsLayout->addStretch();
    scrollArea->setWidget(rowsWidget);
    mainLayout->addWidget(scrollArea, 1);

    auto* previewLabel = new QLabel(tr("Generated Partial Result String:"), this);
    mainLayout->addWidget(previewLabel);
    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setMaximumHeight(100);
    m_preview->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    mainLayout->addWidget(m_preview);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    updatePreview();
}

void PartialModifierDialog::addPartialRow(
    int partialIndex, const PartialModifierFormat::Values& values)
{
    auto* group = new QGroupBox(tr("Partial %1").arg(partialIndex + 1), this);
    auto* layout = new QVBoxLayout(group);

    PartialRow row;
    addEnvelopeEntry(layout, tr("Probability:"), values.probability, true, &row.probability);

    addEnvelopeEntry(layout, tr("Magnitude (cycle depth):"), values.magnitude,
                     true, &row.magnitude);

    addEnvelopeEntry(layout, tr("Width (unused by Phase Modulation):"),
                     values.width, false, &row.width);

    addEnvelopeEntry(layout, tr("Rate (Hz):"), values.rate, true, &row.rate);

    m_rows.append(row);
    m_rowsLayout->addWidget(group);
}

void PartialModifierDialog::addEnvelopeEntry(QVBoxLayout* layout,
                                             const QString& label,
                                             const QString& value,
                                             bool enabled,
                                             QLineEdit** entry)
{
    auto* rowLayout = new QHBoxLayout;
    auto* rowLabel = new QLabel(label, this);
    rowLabel->setMinimumWidth(225);
    auto* lineEdit = new QLineEdit(
        PartialModifierFormat::normalizedValue(value, enabled), this);
    auto* button = new QPushButton(tr("Insert Function"), this);

    lineEdit->setEnabled(enabled);
    button->setEnabled(enabled);
    rowLabel->setEnabled(enabled);
    lineEdit->setToolTip(enabled
        ? tr("An ENV-returning function, or N/A.")
        : tr("This value is not used by the selected modifier type."));

    connect(button, &QPushButton::clicked, this,
            [this, lineEdit]() { openEnvelopeGenerator(lineEdit); });
    connect(lineEdit, &QLineEdit::textChanged, this,
            [this]() { updatePreview(); });

    rowLayout->addWidget(rowLabel);
    rowLayout->addWidget(lineEdit, 1);
    rowLayout->addWidget(button);
    layout->addLayout(rowLayout);
    *entry = lineEdit;
}

void PartialModifierDialog::openEnvelopeGenerator(QLineEdit* entry)
{
    QString original = entry->text().trimmed();
    if (original == QStringLiteral("N/A"))
        original.clear();

    FunctionGenerator generator(this, FunctionReturnType::functionReturnENV, original);
    if (generator.exec() == QDialog::Accepted) {
        const QString result = generator.getResultString().trimmed();
        if (!result.isEmpty())
            entry->setText(result);
    }
}

QString PartialModifierDialog::resultString() const
{
    QVector<PartialModifierFormat::Values> values;
    values.reserve(m_rows.size());
    for (const PartialRow& row : m_rows) {
        PartialModifierFormat::Values value;
        value.probability = row.probability->text();
        value.magnitude = row.magnitude->text();
        value.width = row.width->text();
        value.rate = row.rate->text();
        values.append(value);
    }
    return PartialModifierFormat::serialize(values);
}

void PartialModifierDialog::updatePreview()
{
    if (m_preview)
        m_preview->setPlainText(resultString());
}
