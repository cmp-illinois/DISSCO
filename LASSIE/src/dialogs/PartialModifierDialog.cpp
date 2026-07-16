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
    setWindowTitle("Partial Modifier");
    resize(700, 400);

    m_mainLayout = new QVBoxLayout(this);
    m_rowsLayout = new QVBoxLayout();

    m_mainLayout->addLayout(m_rowsLayout);
    m_mainLayout->addStretch();

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

    QHBoxLayout* rowLayout = new QHBoxLayout();

    QLabel* partialLabel = new QLabel(
        "Partial " + QString::number(m_nodeCount) + ":", this);
    rowLayout->addWidget(partialLabel);

    const QStringList fields = enabledFieldLabels();

    PartialRow row;

    for (const QString& fieldLabel : fields) {
        QLabel* label = new QLabel(fieldLabel, this);
        QLineEdit* edit = new QLineEdit(this);

        edit->setMinimumWidth(120);

        connect(edit, &QLineEdit::cursorPositionChanged,
                this, [this, edit]() {
                    trackFocusedEdit(edit);
                });

        if (fieldLabel == "Probability Envelope:") {
            row.probability = edit;
        } else if (fieldLabel == "Magnitude Envelope:") {
            row.magnitude = edit;
        } else if (fieldLabel == "Rate Value Envelope:") {
            row.rate = edit;
        } else if (fieldLabel == "Width Envelope:") {
            row.width = edit;
        } else if (fieldLabel == "Detune Spread:") {
            row.spread = edit;
        } else if (fieldLabel == "Detune Direction:") {
            row.direction = edit;
        } else if (fieldLabel == "Detune Velocity:") {
            row.velocity = edit;
        }

        rowLayout->addWidget(label);
        rowLayout->addWidget(edit);
    }

    m_rows.append(row);
    m_rowsLayout->addLayout(rowLayout);
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
    if (!edit || edit->text().trimmed().isEmpty()) {
        return "N/A";
    }
    return edit->text();
}
QString PartialModifierDialog::buildPartialResultString() const
{
    QString result = "<Fun><Name>PartialResult</Name><Envelopes>";

    for (const PartialRow& row : m_rows) {
        // CMOD currently reads four envelopes per partial in this fixed order:
        // probability, magnitude/amplitude, width, rate.
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

        if (row.probability && probability != "N/A") {
            row.probability->setText(probability);
        }

        if (row.magnitude && magnitude != "N/A") {
            row.magnitude->setText(magnitude);
        }

        if (row.width && width != "N/A") {
            row.width->setText(width);
        }

        if (row.rate && rate != "N/A") {
            row.rate->setText(rate);
        }
    }
}


QString PartialModifierDialog::getResultString() const
{
    return buildPartialResultString();
}

