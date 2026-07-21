#include "PartialModifierDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>
#include <QStringList>
#include "FunctionGenerator.hpp"
#include <QRegularExpression>
#include <QPlainTextEdit>

using enum FunctionReturnType;

PartialModifierDialog::PartialModifierDialog(
    int modifierType,
    int maxPartialCount,
    const QString& existingResultString,
    QWidget* parent
)
    : QDialog(parent),
      m_modifierType(modifierType),
      m_maxPartialCount(maxPartialCount)
{
    setWindowTitle("Customize Partials");
    resize(950, 500);

    m_mainLayout = new QVBoxLayout(this);
    m_rowsLayout = new QVBoxLayout();

    m_mainLayout->addLayout(m_rowsLayout);
    m_mainLayout->addStretch();

    QLabel* resultLabel = new QLabel("Result String", this);
    m_resultPreview = new QPlainTextEdit(this);
    m_resultPreview->setReadOnly(true);
    m_resultPreview->setMinimumHeight(55);

    m_mainLayout->addWidget(resultLabel);
    m_mainLayout->addWidget(m_resultPreview);

    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_addNodeButton = new QPushButton("Add Node", this);
    m_insertFunctionButton = new QPushButton("Insert Function", this);
    m_okButton = new QPushButton("OK", this);
    m_cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_addNodeButton);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(m_insertFunctionButton);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();

    m_mainLayout->addLayout(buttonLayout);

    connect(m_addNodeButton, &QPushButton::clicked,
            this, &PartialModifierDialog::addNode);

    connect(m_insertFunctionButton, &QPushButton::clicked,
            this, &PartialModifierDialog::insertFunction);

    connect(m_okButton, &QPushButton::clicked,
            this, &QDialog::accept);

    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    populateFromResultString(existingResultString);
    updateResultPreview();
}


void PartialModifierDialog::addNode()
{
    if (m_maxPartialCount <= 0) {
        QMessageBox::warning(
            this,
            "No Spectrum",
            "This Bottom event does not contain a Spectrum, so partial nodes cannot be added."
        );
        return;
    }

    if (m_nodeCount >= m_maxPartialCount) {
        QMessageBox::warning(
            this,
            "Partial Limit Reached",
            "Cannot add more partial nodes than the maximum number of partials in this Bottom event's spectra."
        );
        return;
    }

    ++m_nodeCount;

    QWidget* rowContainer = new QWidget(this);
    QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* partialLabel = new QLabel(
        "Partial #" + QString::number(m_nodeCount), this);
    rowLayout->addWidget(partialLabel);

    PartialRow row;
    row.container = rowContainer;
    row.partialLabel = partialLabel;

    const EnvelopeEnabled enabled = enabledEnvelopeFields();

    row.probabilityLabel = new QLabel("Probability:", this);
    row.probability = new QLineEdit(this);
    row.probability->setText(enabled.probability ? "PROB" : "");
    row.probability->setMinimumWidth(120);

    row.magnitudeLabel = new QLabel("Magnitude:", this);
    row.magnitude = new QLineEdit(this);
    row.magnitude->setText(enabled.magnitude ? "AMP" : "");
    row.magnitude->setMinimumWidth(120);

    row.widthLabel = new QLabel("Width:", this);
    row.width = new QLineEdit(this);
    row.width->setText(enabled.width ? "WIDTH" : "");
    row.width->setMinimumWidth(120);

    row.rateLabel = new QLabel("Rate Value:", this);
    row.rate = new QLineEdit(this);
    row.rate->setText(enabled.rate ? "RATE" : "");
    row.rate->setMinimumWidth(120);

    connect(row.probability, &QLineEdit::cursorPositionChanged,
            this, [this, row]() {
                trackFocusedEdit(row.probability);
            });

    connect(row.magnitude, &QLineEdit::cursorPositionChanged,
            this, [this, row]() {
                trackFocusedEdit(row.magnitude);
            });

    connect(row.width, &QLineEdit::cursorPositionChanged,
            this, [this, row]() {
                trackFocusedEdit(row.width);
            });

    connect(row.rate, &QLineEdit::cursorPositionChanged,
            this, [this, row]() {
                trackFocusedEdit(row.rate);
            });
    connect(row.probability, &QLineEdit::textChanged,
        this, &PartialModifierDialog::updateResultPreview);

    connect(row.magnitude, &QLineEdit::textChanged,
            this, &PartialModifierDialog::updateResultPreview);

    connect(row.width, &QLineEdit::textChanged,
            this, &PartialModifierDialog::updateResultPreview);

    connect(row.rate, &QLineEdit::textChanged,
            this, &PartialModifierDialog::updateResultPreview);
    
    rowLayout->addWidget(row.probabilityLabel);
    rowLayout->addWidget(row.probability);

    rowLayout->addWidget(row.magnitudeLabel);
    rowLayout->addWidget(row.magnitude);

    rowLayout->addWidget(row.widthLabel);
    rowLayout->addWidget(row.width);

    rowLayout->addWidget(row.rateLabel);
    rowLayout->addWidget(row.rate);

    QPushButton* removeButton = new QPushButton("Remove Node", this);
    connect(removeButton, &QPushButton::clicked,
        this, [this, rowContainer]() {
            removeNode(rowContainer);
        });

    rowLayout->addWidget(removeButton);

    applyRowEnabledState(row);

    m_rows.append(row);
    m_rowsLayout->addWidget(rowContainer);
    updateResultPreview();
}


void PartialModifierDialog::insertFunction()
{
    if (!m_lastFocusedEdit) {
        QMessageBox::warning(this,
                             "No Field Selected",
                             "Please select a field before inserting a function.");
        return;
    }

    FunctionGenerator* gen = new FunctionGenerator(
        this,
        functionReturnENV,
        m_lastFocusedEdit->text()
    );

    if (gen) {
        if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty()) {
            m_lastFocusedEdit->setText(gen->getResultString());
        }
        delete gen;
    }
}

QStringList PartialModifierDialog::enabledFieldLabels() const
{
    // Field order:
    // Probability, Magnitude, Rate, Width, Detune Spread, Detune Direction, Detune Velocity
    static const bool table[7][7] = {
        /* TREMOLO   */ { true,  true,  true,  false, false, false, false },
        /* VIBRATO   */ { true,  true,  true,  false, false, false, false },
        /* GLISSANDO */ { true,  true,  false, false, false, false, false },
        /* DETUNE    */ { true,  false, false, false, true,  true,  true  },
        /* AMPTRANS  */ { true,  true,  true,  true,  false, false, false },
        /* FREQTRANS */ { true,  true,  true,  true,  false, false, false },
        /* WAVE_TYPE */ { true,  true,  true,  false, false, false, false },
    };

    const QString labels[7] = {
        "Probability Envelope:",
        "Magnitude Envelope:",
        "Rate Value Envelope:",
        "Width Envelope:",
        "Detune Spread:",
        "Detune Direction:",
        "Detune Velocity:"
    };

    QStringList result;

    if (m_modifierType < 0 || m_modifierType >= 7) {
        return result;
    }

    for (int i = 0; i < 7; ++i) {
        if (table[m_modifierType][i]) {
            result.append(labels[i]);
        }
    }

    return result;
}

void PartialModifierDialog::trackFocusedEdit(QLineEdit* edit)
{
    m_lastFocusedEdit = edit;
}



QString PartialModifierDialog::envelopeOrNA(QLineEdit* edit)
{
    if (!edit) {
        return "";
    }

    return edit->text();
}

QString PartialModifierDialog::buildPartialResultString() const
{
    QString result = "<Fun><Name>Partials</Name><Envelopes>";

    for (const PartialRow& row : m_rows) {
        result += "<Envelope>" + envelopeOrNA(row.probability) + "</Envelope>";
        result += "<Envelope>" + envelopeOrNA(row.magnitude) + "</Envelope>";
        result += "<Envelope>" + envelopeOrNA(row.width) + "</Envelope>";
        result += "<Envelope>" + envelopeOrNA(row.rate) + "</Envelope>";
    }

    result += "</Envelopes></Fun>";
    return result;
}

QStringList PartialModifierDialog::extractEnvelopeValues(const QString& resultString)
{
    QStringList values;

    QRegularExpression re("<Envelope>([\\s\\S]*?)</Envelope>");
    QRegularExpressionMatchIterator it = re.globalMatch(resultString);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        values.append(match.captured(1));
    }

    return values;
}

void PartialModifierDialog::populateFromResultString(const QString& resultString)
{
    if (resultString.trimmed().isEmpty()) {
        return;
    }

    const QStringList envelopes = extractEnvelopeValues(resultString);

    if (envelopes.isEmpty()) {
        return;
    }

    // CMOD currently reads 4 envelopes per partial:
    // probability, magnitude, width, rate
    const int valuesPerPartial = 4;
    const int partialCount = envelopes.size() / valuesPerPartial;

    for (int i = 0; i < partialCount; ++i) {
        const int oldRowCount = m_rows.size();

        addNode();

        if (m_rows.size() == oldRowCount) {
            return;
        }

        PartialRow& row = m_rows.last();

        const QString probability = envelopes[i * valuesPerPartial + 0];
        const QString magnitude   = envelopes[i * valuesPerPartial + 1];
        const QString width       = envelopes[i * valuesPerPartial + 2];
        const QString rate        = envelopes[i * valuesPerPartial + 3];

        if (row.probability) {
            row.probability->setText(probability);
        }

        if (row.magnitude) {
            row.magnitude->setText(magnitude);
        }

        if (row.width) {
            row.width->setText(width);
        }

        if (row.rate) {
            row.rate->setText(rate);
        }

        applyRowEnabledState(row);
    }
}

PartialModifierDialog::EnvelopeEnabled PartialModifierDialog::enabledEnvelopeFields() const
{
    EnvelopeEnabled enabled;

    switch (m_modifierType) {
        case 0: // TREMOLO
            enabled.probability = true;
            enabled.magnitude = true;
            enabled.width = false;
            enabled.rate = true;
            break;

        case 1: // VIBRATO
            enabled.probability = true;
            enabled.magnitude = true;
            enabled.width = false;
            enabled.rate = true;
            break;

        case 2: // GLISSANDO
            enabled.probability = true;
            enabled.magnitude = true;
            enabled.width = false;
            enabled.rate = false;
            break;

        case 4: // AMPTRANS
            enabled.probability = true;
            enabled.magnitude = true;
            enabled.width = true;
            enabled.rate = true;
            break;

        case 5: // FREQTRANS
            enabled.probability = true;
            enabled.magnitude = true;
            enabled.width = true;
            enabled.rate = true;
            break;

        case 6: // WAVE_TYPE
            enabled.probability = true;
            enabled.magnitude = true;
            enabled.width = false;
            enabled.rate = true;
            break;

        default:
            enabled.probability = true;
            enabled.magnitude = true;
            enabled.width = true;
            enabled.rate = true;
            break;
    }

    return enabled;
}

void PartialModifierDialog::applyRowEnabledState(const PartialRow& row) const
{
    const EnvelopeEnabled enabled = enabledEnvelopeFields();

    auto apply = [](QLabel* label, QLineEdit* edit, bool isEnabled) {
        if (label) {
            label->setEnabled(isEnabled);
        }

        if (edit) {
            edit->setEnabled(isEnabled);

            if (!isEnabled) {
                edit->clear();
            }
        }
    };

    apply(row.probabilityLabel, row.probability, enabled.probability);
    apply(row.magnitudeLabel, row.magnitude, enabled.magnitude);
    apply(row.widthLabel, row.width, enabled.width);
    apply(row.rateLabel, row.rate, enabled.rate);
}

void PartialModifierDialog::removeNode(QWidget* rowContainer)
{
    if (!rowContainer) {
        return;
    }

    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].container == rowContainer) {
            m_rows.removeAt(i);
            break;
        }
    }

    if (m_lastFocusedEdit && rowContainer->isAncestorOf(m_lastFocusedEdit)) {
        m_lastFocusedEdit = nullptr;
    }

    m_rowsLayout->removeWidget(rowContainer);
    rowContainer->deleteLater();

    m_nodeCount = m_rows.size();
    renumberRows();
    updateResultPreview();
}

void PartialModifierDialog::renumberRows()
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].partialLabel) {
            m_rows[i].partialLabel->setText(
                "Partial #" + QString::number(i + 1)
            );
        }
    }
}

void PartialModifierDialog::updateResultPreview()
{
    if (!m_resultPreview) {
        return;
    }

    m_resultPreview->setPlainText(buildPartialResultString());
}

QString PartialModifierDialog::getResultString() const
{
    return buildPartialResultString();
}

