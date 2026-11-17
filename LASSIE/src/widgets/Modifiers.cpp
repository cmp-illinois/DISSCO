#include "Modifiers.hpp"

#include "../dialogs/FunctionGenerator.hpp"
#include "../dialogs/ModifierDetailsDialog.hpp"
#include "../dialogs/ModifierRulesDialog.hpp"
#include "../inst.hpp"
#include "../ui/ui_Modifiers.h"
#include "ModifierUiPolicy.hpp"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QUuid>

namespace {

// Display order differs from the stable integer codes serialized for CMOD.
constexpr int modifierTypesByDisplayOrder[] = {0, 1, 2, 3, 7, 4, 5, 6};
constexpr int modifierTypeCount =
    sizeof(modifierTypesByDisplayOrder) / sizeof(modifierTypesByDisplayOrder[0]);

QString normalizedChance(double percent)
{
    return QString::number(percent / 100.0, 'g', 15);
}

} // namespace

Modifiers::Modifiers(Eventtype eventType, unsigned eventIndex,
                     int modifierIndex, QWidget* parent)
    : QFrame(parent),
      ui(new Ui::Modifiers),
      m_eventType(eventType),
      m_eventIndex(eventIndex),
      m_modifierIndex(modifierIndex)
{
    ui->setupUi(this);
    QSizePolicy rowPolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    rowPolicy.setHeightForWidth(true);
    setSizePolicy(rowPolicy);

    for (int index = 0; index < modifierTypeCount; ++index)
        ui->modifierType->setItemData(index, modifierTypesByDisplayOrder[index]);

    Modifier& modifier = backendModifier();
    if (modifier.instance_id.trimmed().isEmpty()) {
        modifier.instance_id =
            QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    connect(ui->modifierRemoveButton, &QPushButton::clicked,
            this, [this]() { emit deleteRequested(this); });
    connect(ui->moveUpButton, &QPushButton::clicked,
            this, [this]() { emit moveUpRequested(this); });
    connect(ui->moveDownButton, &QPushButton::clicked,
            this, [this]() { emit moveDownRequested(this); });
    connect(ui->parametersButton, &QPushButton::clicked,
            this, [this]() { openParameters(); });
    connect(ui->rulesButton, &QPushButton::clicked,
            this, [this]() { openRules(); });

    m_defaultEnvelopeButton = new QPushButton(this);
    m_defaultEnvelopeButton->setObjectName(
        QStringLiteral("defaultChanceEnvelopeButton"));
    m_defaultEnvelopeButton->setToolTip(
        tr("Use an envelope so the Default ON chance changes over time. It is "
           "read on the timeline of the event that contains the Bottom: at "
           "each sound's start (Per sound) or at the Bottom's start "
           "(Per bottom)."));
    m_defaultNumberButton = new QPushButton(tr("Use number"), this);
    m_defaultNumberButton->setObjectName(
        QStringLiteral("defaultChanceNumberButton"));
    m_defaultNumberButton->setToolTip(
        tr("Remove the envelope and use a fixed Default ON chance."));
    ui->defaultChanceLayout->addWidget(m_defaultEnvelopeButton);
    ui->defaultChanceLayout->addWidget(m_defaultNumberButton);
    connect(m_defaultEnvelopeButton, &QPushButton::clicked,
            this, [this]() { openDefaultOnEnvelope(); });
    connect(m_defaultNumberButton, &QPushButton::clicked,
            this, [this]() { useDefaultOnNumber(); });

    connect(ui->modifierType,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                const int type = currentModifierType();
                if (type >= 0)
                    backendModifier().type = static_cast<unsigned>(type);
                updateRow();
                emit dataChanged();
            });
    connect(ui->defaultChanceSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                backendModifier().default_on_chance =
                    normalizedChance(value);
                backendModifier().usage_metadata_needs_review = false;
                emit dataChanged();
            });

    updateRow();
}

QList<Modifier>& Modifiers::backendModifierList()
{
    ProjectManager* projectManager = Inst::get_project_manager();
    if (m_eventType == top)
        return projectManager->topevent().modifiers;
    if (m_eventType == high)
        return projectManager->highevents()[m_eventIndex].modifiers;
    if (m_eventType == mid)
        return projectManager->midevents()[m_eventIndex].modifiers;
    if (m_eventType == low)
        return projectManager->lowevents()[m_eventIndex].modifiers;
    return projectManager->bottomevents()[m_eventIndex].extra_info.modifiers;
}

Modifier& Modifiers::backendModifier()
{
    return backendModifierList()[m_modifierIndex];
}

int Modifiers::currentModifierType() const
{
    bool valid = false;
    const int type = ui->modifierType->currentData().toInt(&valid);
    return valid ? type : -1;
}

void Modifiers::setModifierIndex(int modifierIndex)
{
    m_modifierIndex = modifierIndex;
    updateRow();
}

void Modifiers::saveModifierToBackend()
{
    const int type = currentModifierType();
    if (type >= 0)
        backendModifier().type = static_cast<unsigned>(type);
    backendModifier().default_on_chance =
        normalizedChance(ui->defaultChanceSpin->value());
}

void Modifiers::updateRow()
{
    const Modifier& modifier = backendModifier();

    ui->orderLabel->setText(QStringLiteral("%1.").arg(m_modifierIndex + 1));
    ui->moveUpButton->setEnabled(m_modifierIndex > 0);
    ui->moveDownButton->setEnabled(
        m_modifierIndex + 1 < backendModifierList().size());

    {
        const QSignalBlocker blocker(ui->modifierType);
        ui->modifierType->setCurrentIndex(
            ui->modifierType->findData(static_cast<int>(modifier.type)));
    }

    bool validChance = false;
    const double chance = modifier.default_on_chance.toDouble(&validChance);
    {
        const QSignalBlocker blocker(ui->defaultChanceSpin);
        ui->defaultChanceSpin->setValue(
            validChance ? chance * 100.0 : 100.0);
    }
    const bool hasEnvelope =
        !modifier.default_on_envelope.trimmed().isEmpty();
    ui->defaultChanceSpin->setVisible(!hasEnvelope);
    m_defaultEnvelopeButton->setText(
        hasEnvelope ? tr("Edit envelope...") : tr("Envelope..."));
    m_defaultNumberButton->setVisible(hasEnvelope);

    const int ruleCount = modifier.rules.size();
    ui->rulesButton->setText(
        ruleCount == 0
            ? tr("No exceptions")
            : tr("%1 exception%2")
                  .arg(ruleCount)
                  .arg(ruleCount == 1 ? QString() : QStringLiteral("s")));

    ui->parametersButton->setText(tr("Parameters..."));
    ui->parametersButton->setToolTip(
        tr("Edit Apply To, Magnitude, Rate, Width, Detune, and partial "
           "values for %1.")
            .arg(ModifierUiPolicy::displayName(
                static_cast<int>(modifier.type))));
}

void Modifiers::openParameters()
{
    ModifierDetailsDialog dialog(
        backendModifier(), m_eventType, m_eventIndex, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    backendModifier() = dialog.resultModifier();
    updateRow();
    emit dataChanged();
}

void Modifiers::openDefaultOnEnvelope()
{
    FunctionGenerator generator(this, FunctionReturnType::functionReturnENV,
                                backendModifier().default_on_envelope);
    if (generator.exec() != QDialog::Accepted)
        return;
    const QString envelope = generator.getResultString().trimmed();
    if (envelope.isEmpty())
        return;

    backendModifier().default_on_envelope = envelope;
    backendModifier().usage_metadata_needs_review = false;
    updateRow();
    emit dataChanged();
}

void Modifiers::useDefaultOnNumber()
{
    backendModifier().default_on_envelope.clear();
    updateRow();
    emit dataChanged();
}

void Modifiers::openRules()
{
    QList<Modifier> earlier;
    const QList<Modifier>& modifiers = backendModifierList();
    for (int index = 0; index < m_modifierIndex; ++index)
        earlier.append(modifiers[index]);

    ModifierRulesDialog dialog(backendModifier(), earlier, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    backendModifier().rules = dialog.resultRules();
    backendModifier().usage_metadata_needs_review = false;
    updateRow();
    emit dataChanged();
}

Modifiers::~Modifiers()
{
    delete ui;
}
