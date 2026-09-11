#include "PartialModifierDialog.hpp"

#include "FunctionGenerator.hpp"
#include "FunctionXmlFormat.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>

namespace {

struct ParameterPresentation {
    QString magnitudeLabel;
    QString widthLabel;
    QString rateLabel;
    bool usesMagnitude = false;
    bool usesWidth = false;
    bool usesRate = false;
};

ParameterPresentation presentationFor(int modifierType)
{
    switch (modifierType) {
    case 0:
        return {QObject::tr("Magnitude (depth):"), QString(),
                QObject::tr("Rate (Hz):"), true, false, true};
    case 1:
        return {QObject::tr("Magnitude (frequency depth):"), QString(),
                QObject::tr("Rate (Hz):"), true, false, true};
    case 2:
        return {QObject::tr("Frequency change:"), QString(), QString(),
                true, false, false};
    case 3:
        return {QObject::tr("Detuning:"), QString(), QString(),
                true, false, false};
    case 4:
    case 5:
        return {QObject::tr("Magnitude:"), QObject::tr("Width:"),
                QObject::tr("Rate:"), true, true, true};
    case 6:
        return {QObject::tr("Wave type:"), QString(), QString(),
                true, false, false};
    case 7:
        return {QObject::tr("Magnitude (cycle depth):"), QString(),
                QObject::tr("Rate (Hz):"), true, false, true};
    default:
        return {QObject::tr("Magnitude:"), QObject::tr("Width:"),
                QObject::tr("Rate:"), true, true, true};
    }
}

QString unusedLabel(const QString& parameter)
{
    return QObject::tr("%1 (not used by this modifier):").arg(parameter);
}

} // namespace

PartialModifierDialog::PartialModifierDialog(QWidget* parent,
                                             int modifierType,
                                             const ModifierUiPolicy::PartialRowConstraint& constraint,
                                             const QString& originalString)
    : QDialog(parent),
      m_modifierType(modifierType),
      m_constraint(constraint)
{
    setWindowTitle(tr("Customize Partials"));
    setModal(true);
    resize(900, 700);

    auto* mainLayout = new QVBoxLayout(this);
    auto* explanation = new QLabel(
        tr("Each row controls one spectrum partial. Probability decides whether "
           "the modifier is applied to that partial. Use Insert Function to build "
           "each envelope; N/A means that value is not used."),
        this);
    explanation->setWordWrap(true);
    mainLayout->addWidget(explanation);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    QString parseWarning;
    QVector<PartialModifierFormat::Values> values =
        PartialModifierFormat::parse(originalString, &parseWarning);
    m_savedRowCount = values.size();
    const int requestedRows = std::max(1, constraint.suggestedRows);
    m_suggestedPartialCount = requestedRows;
    // Opening the editor must not silently add rows to an existing value.
    // Start with the saved count (or one blank row) and let the explicit Add
    // action grow it toward the Spectrum-derived suggestion/limit.
    const int rowCount = std::max(1, static_cast<int>(values.size()));
    values.resize(rowCount);
    m_activeRowCount = rowCount;

    QStringList statusParts;
    if (!constraint.explanation.isEmpty())
        statusParts.append(constraint.explanation);
    if (!parseWarning.isEmpty()) {
        statusParts.append(parseWarning
                           + tr(" Cancel preserves the original value."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #b06000;"));
    }
    m_statusContext = statusParts.join(QStringLiteral("\n"));

    auto* countLayout = new QHBoxLayout;
    auto* countLabel = new QLabel(tr("Number of partial rows:"), this);
    m_rowCountSpin = new QSpinBox(this);
    m_rowCountSpin->setObjectName(QStringLiteral("partialRowCountSpin"));
    m_rowCountSpin->setKeyboardTracking(false);
    m_rowCountSpin->setReadOnly(true);
    m_rowCountSpin->setRange(
        1, ModifierUiPolicy::editorRowMaximum(constraint, m_savedRowCount));
    m_rowCountSpin->setValue(rowCount);
    countLabel->setBuddy(m_rowCountSpin);
    m_addPartialButton = new QPushButton(tr("Add partial"), this);
    m_addPartialButton->setObjectName(QStringLiteral("addPartialButton"));
    m_removePartialButton = new QPushButton(tr("Remove last partial"), this);
    m_removePartialButton->setObjectName(QStringLiteral("removePartialButton"));
    countLayout->addWidget(countLabel);
    countLayout->addWidget(m_rowCountSpin);
    countLayout->addWidget(m_addPartialButton);
    countLayout->addWidget(m_removePartialButton);
    countLayout->addStretch();
    mainLayout->addLayout(countLayout);

    m_countWarningLabel = new QLabel(this);
    m_countWarningLabel->setWordWrap(true);
    m_countWarningLabel->setStyleSheet(QStringLiteral("color: #b06000;"));
    m_countWarningLabel->setVisible(false);
    mainLayout->addWidget(m_countWarningLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    auto* rowsWidget = new QWidget(m_scrollArea);
    m_rowsLayout = new QVBoxLayout(rowsWidget);
    m_rowsLayout->addStretch();
    for (int i = 0; i < rowCount; ++i)
        addPartialRow(i, values.at(i));
    m_scrollArea->setWidget(rowsWidget);
    mainLayout->addWidget(m_scrollArea, 1);

    auto* previewLabel = new QLabel(tr("Generated Partial Result String:"), this);
    mainLayout->addWidget(previewLabel);
    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setMaximumHeight(100);
    m_preview->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_preview->setToolTip(tr("Generated expression on one line. Scroll horizontally to read it; edit the partial rows above to change values."));
    mainLayout->addWidget(m_preview);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted,
            this, &PartialModifierDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    connect(m_rowCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int count) { setPartialRowCount(count); });
    connect(m_addPartialButton, &QPushButton::clicked, this, [this]() {
        m_rowCountSpin->setValue(m_activeRowCount + 1);
    });
    connect(m_removePartialButton, &QPushButton::clicked, this, [this]() {
        m_rowCountSpin->setValue(m_activeRowCount - 1);
    });

    updateCountControls();
    updatePreview();
}

void PartialModifierDialog::addPartialRow(
    int partialIndex, const PartialModifierFormat::Values& values)
{
    auto* group = new QGroupBox(tr("Partial %1").arg(partialIndex + 1), this);
    auto* layout = new QVBoxLayout(group);
    const ParameterPresentation presentation = presentationFor(m_modifierType);

    PartialRow row;
    row.group = group;
    addEnvelopeEntry(layout, tr("Probability:"), values.probability, true, &row.probability);

    addEnvelopeEntry(
        layout,
        presentation.usesMagnitude
            ? presentation.magnitudeLabel : unusedLabel(tr("Magnitude")),
        values.magnitude, presentation.usesMagnitude, &row.magnitude);

    addEnvelopeEntry(
        layout,
        presentation.usesWidth
            ? presentation.widthLabel : unusedLabel(tr("Width")),
        values.width, presentation.usesWidth, &row.width);

    addEnvelopeEntry(
        layout,
        presentation.usesRate
            ? presentation.rateLabel : unusedLabel(tr("Rate")),
        values.rate, presentation.usesRate, &row.rate);

    m_rows.append(row);
    m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, group);
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
        // A field can be unused by the current modifier type yet become
        // meaningful if that type is changed later. Keep its saved slot while
        // disabling the control instead of silently replacing it with N/A.
        PartialModifierFormat::normalizedValue(value, true), this);
    auto* button = new QPushButton(tr("Insert Function"), this);

    lineEdit->setEnabled(enabled);
    button->setEnabled(enabled);
    rowLabel->setEnabled(enabled);
    rowLabel->setBuddy(lineEdit);
    button->setAccessibleName(tr("Insert function for %1").arg(label));
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

void PartialModifierDialog::setPartialRowCount(int count)
{
    const int previousCount = m_activeRowCount;
    while (m_rows.size() < count)
        addPartialRow(m_rows.size(), PartialModifierFormat::Values{});

    if (count < previousCount) {
        for (int index = count; index < previousCount; ++index) {
            PartialRow& row = m_rows[index];
            row.probability->setText(QStringLiteral("N/A"));
            row.magnitude->setText(QStringLiteral("N/A"));
            row.width->setText(QStringLiteral("N/A"));
            row.rate->setText(QStringLiteral("N/A"));
        }
    }

    m_activeRowCount = count;
    for (int index = 0; index < m_rows.size(); ++index)
        m_rows[index].group->setVisible(index < m_activeRowCount);
    updateCountControls();
    updatePreview();

    if (count > previousCount && count <= m_rows.size()) {
        PartialRow& row = m_rows[count - 1];
        row.probability->setFocus();
        if (m_scrollArea)
            m_scrollArea->ensureWidgetVisible(row.group);
    }
}

QString PartialModifierDialog::resultString() const
{
    QVector<PartialModifierFormat::Values> values;
    values.reserve(m_activeRowCount);
    for (int index = 0; index < m_activeRowCount; ++index) {
        const PartialRow& row = m_rows[index];
        PartialModifierFormat::Values value;
        value.probability = row.probability->text();
        value.magnitude = row.magnitude->text();
        value.width = row.width->text();
        value.rate = row.rate->text();
        values.append(value);
    }
    return PartialModifierFormat::serialize(values);
}

void PartialModifierDialog::accept()
{
    if (!ModifierUiPolicy::rowCountAllowed(
            m_constraint, m_activeRowCount, m_savedRowCount)) {
        QMessageBox::warning(
            this, tr("Too many partial rows"),
            tr("This configuration cannot grow beyond %1 rows. Reduce the row "
               "count or cancel to preserve the original value.")
                .arg(std::max(m_constraint.maximumRows, m_savedRowCount)));
        return;
    }

    const QString error = PartialModifierFormat::validationError(
        m_modifierType, resultString());
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid partial parameters"), error);
        return;
    }
    QDialog::accept();
}

void PartialModifierDialog::updateCountControls()
{
    if (!m_rowCountSpin)
        return;

    const bool hasExactLimit = m_constraint.maximumRows > 0;
    const bool aboveCurrentLimit =
        hasExactLimit && m_activeRowCount > m_constraint.maximumRows;
    const bool belowSuggestion =
        m_activeRowCount < m_suggestedPartialCount;

    QString status = m_statusContext;
    if (!status.isEmpty())
        status += QLatin1Char('\n');
    status += tr("Showing %1 partial row(s).").arg(m_activeRowCount);
    m_statusLabel->setText(status);

    m_addPartialButton->setEnabled(
        m_activeRowCount < m_rowCountSpin->maximum()
        && (!hasExactLimit
            || m_activeRowCount < m_constraint.maximumRows));
    m_removePartialButton->setEnabled(m_activeRowCount > 1);

    if (aboveCurrentLimit) {
        m_countWarningLabel->setText(tr(
            "This saved modifier has %1 rows, while the current Spectrum limit "
            "is %2. The extra rows are preserved but CMOD ignores them for these "
            "sounds; no additional rows can be added.")
            .arg(m_activeRowCount)
            .arg(m_constraint.maximumRows));
        m_countWarningLabel->setVisible(true);
    } else if (belowSuggestion) {
        m_countWarningLabel->setText(tr(
            "Partials after row %1 will not receive this modifier.")
            .arg(m_activeRowCount));
        m_countWarningLabel->setVisible(true);
    } else {
        m_countWarningLabel->setVisible(false);
    }
}

void PartialModifierDialog::updatePreview()
{
    if (m_preview)
        m_preview->setPlainText(FunctionXmlFormat::compact(resultString()));
}
