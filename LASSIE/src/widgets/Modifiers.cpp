#include "Modifiers.hpp"
#include "../ui/ui_Modifiers.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QDialog>
#include <QSizePolicy>

#include <algorithm>
#include <string>

#include "../ui/ui_Attributes.h"
#include "../inst.hpp"
#include "../dialogs/FunctionGenerator.hpp"
#include "../dialogs/PartialModifierDialog.hpp"
#include "../dialogs/GeneralPartialModifierDialog.hpp"
#include "ModifierUiPolicy.hpp"


using enum FunctionReturnType;

namespace {
constexpr int phaseModType = 7;
constexpr int detuneType = 3;

// The combo box is ordered for display, while these values must retain the
// stable modifier IDs serialized in project files and consumed by CMOD.
constexpr int modifierTypesByDisplayOrder[] = { 0, 1, 2, 3, 7, 4, 5, 6 };
constexpr int modifierTypeCount =
    sizeof(modifierTypesByDisplayOrder) / sizeof(modifierTypesByDisplayOrder[0]);

int currentModifierType(const QComboBox* combo)
{
    bool valid = false;
    const int type = combo->currentData().toInt(&valid);
    return valid ? type : -1;
}

void saveCurrentModifierType(const QComboBox* combo, Modifier& modifier)
{
    const int type = currentModifierType(combo);
    if (type >= 0)
        modifier.type = static_cast<unsigned>(type);
}
}

Modifiers::Modifiers(Eventtype eventType, unsigned eventIndex, int modifierIndex, QWidget *parent)
    : QFrame(parent),
      ui(new Ui::Modifiers),
      m_eventType(eventType),
      m_eventIndex(eventIndex),
      m_modifierIndex(modifierIndex)
{
    ui->setupUi(this);
    for (int index = 0; index < modifierTypeCount; ++index)
        ui->modifierType->setItemData(index, modifierTypesByDisplayOrder[index]);

    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    this->setMinimumHeight(0);

    setupUi();
    ui->modifierSpreadLabel->adjustSize();
    ui->modifierDirLabel->adjustSize();
    ui->modifierVelLabel->adjustSize();
    connect(ui->modifierRemoveButton, &QPushButton::clicked,
            this, &Modifiers::modRemoveButtonClicked);

    // Populate UI from backend; setModifierData calls updateModState at the end
    Modifier& modData = getBackendLayer();
    setModifierData(modData);
}

Modifier& Modifiers::getBackendLayer() {
    ProjectManager* pm = Inst::get_project_manager();
    
    if (m_eventType != bottom) {
        HEvent* hevent = nullptr;
        if (m_eventType == top)
            hevent = &pm->topevent();
        else if (m_eventType == high)
            hevent = &pm->highevents()[m_eventIndex];
        else if (m_eventType == mid)
            hevent = &pm->midevents()[m_eventIndex];
        else if (m_eventType == low)
            hevent = &pm->lowevents()[m_eventIndex];

        return hevent->modifiers[m_modifierIndex];
    } else {
        ExtraInfo* bevent = &pm->bottomevents()[m_eventIndex].extra_info;
        return bevent->modifiers[m_modifierIndex];
    }
}

void Modifiers::modRemoveButtonClicked() {
    emit deleteRequested(this);
}

void Modifiers::setupUi() {
    // connecting buttons
    connect(ui->modifierProbFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modProbabilityChanged);
                    });
    connect(ui->modifierMagFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modMagnitudeChanged);
                    });
    connect(ui->modifierRateFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modRateChanged);
                    });
    connect(ui->modifierWidthFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modWidthChanged);
                    });
    connect(ui->modifierResFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modPartialChanged);
                    });
    connect(ui->modifierSpreadFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modSpreadChanged);
                    });
    connect(ui->modifierDirFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modDirChanged);
                    });
    connect(ui->modifierVelFunButton, &QPushButton::clicked,
                    this, [this]() {
                        modFunctionButtonClicked(modVelChanged);
                    });

    // connecting line edits
    connect(ui->modifierNameEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modNameChanged);
                    });
    connect(ui->modifierProbEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modProbabilityChanged);
                    });
    connect(ui->modifierMagEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modMagnitudeChanged);
                    });
    connect(ui->modifierRateEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modRateChanged);
                    });
    connect(ui->modifierWidthEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modWidthChanged);
                    });
    connect(ui->modifierResEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modPartialChanged);
                    });
    connect(ui->modifierSpreadEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modSpreadChanged);
                    });
    connect(ui->modifierDirEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modDirChanged);
                    });
    connect(ui->modifierVelEdit, &QLineEdit::textChanged,
                    this, [this]() {
                        modTextChanged(modVelChanged);
                    });
    
    // connecting combobox
    connect(ui->modifierType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                saveCurrentModifierType(ui->modifierType, getBackendLayer());
                updateApplyOptionsForModifierType();
                updateFieldsForApplyMode();
                updateModState();
            });
    connect(ui->modifierApply, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                getBackendLayer().applyhow_flag = (index == 1);
                updateFieldsForApplyMode();
                updateModState();
            });
    // Reduce spacing between modifier rows.
    ui->modifierGroupLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierGroupLayout->setSpacing(0);

    ui->modifierLayout->setContentsMargins(8, 8, 8, 8);
    ui->modifierLayout->setSpacing(4);

    ui->modifierRemoveLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierRemoveLayout->setSpacing(6);

    ui->modifierNameLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierNameLayout->setSpacing(6);

    ui->modifierProbLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierProbLayout->setSpacing(6);

    ui->modifierMagLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierMagLayout->setSpacing(6);

    ui->modifierRateLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierRateLayout->setSpacing(6);

    ui->modifierWidthLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierWidthLayout->setSpacing(6);

    ui->modifierDetuneLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierDetuneLayout->setSpacing(6);

    ui->modifierResLayout->setContentsMargins(0, 0, 0, 0);
    ui->modifierResLayout->setSpacing(6);
}


// 3 Helpers for switching modifier fields between SOUND and PARTIAL modes
// without accidentally overwriting backend values.
void Modifiers::setSoundFieldsFromBackend()
{
    Modifier& mod = getBackendLayer();

    ui->modifierProbEdit->blockSignals(true);
    ui->modifierProbEdit->setText(mod.probability);
    ui->modifierProbEdit->blockSignals(false);

    ui->modifierMagEdit->blockSignals(true);
    ui->modifierMagEdit->setText(mod.amplitude);
    ui->modifierMagEdit->blockSignals(false);

    ui->modifierRateEdit->blockSignals(true);
    ui->modifierRateEdit->setText(mod.rate);
    ui->modifierRateEdit->blockSignals(false);

    ui->modifierWidthEdit->blockSignals(true);
    ui->modifierWidthEdit->setText(mod.width);
    ui->modifierWidthEdit->blockSignals(false);

    ui->modifierSpreadEdit->blockSignals(true);
    ui->modifierSpreadEdit->setText(mod.detune_spread);
    ui->modifierSpreadEdit->blockSignals(false);

    ui->modifierDirEdit->blockSignals(true);
    ui->modifierDirEdit->setText(mod.detune_direction);
    ui->modifierDirEdit->blockSignals(false);

    ui->modifierVelEdit->blockSignals(true);
    ui->modifierVelEdit->setText(mod.detune_velocity);
    ui->modifierVelEdit->blockSignals(false);
}

void Modifiers::clearSoundFieldsForPartialMode()
{
    ui->modifierProbEdit->blockSignals(true);
    ui->modifierProbEdit->clear();
    ui->modifierProbEdit->blockSignals(false);

    ui->modifierMagEdit->blockSignals(true);
    ui->modifierMagEdit->clear();
    ui->modifierMagEdit->blockSignals(false);

    ui->modifierRateEdit->blockSignals(true);
    ui->modifierRateEdit->clear();
    ui->modifierRateEdit->blockSignals(false);

    ui->modifierWidthEdit->blockSignals(true);
    ui->modifierWidthEdit->clear();
    ui->modifierWidthEdit->blockSignals(false);

    ui->modifierSpreadEdit->blockSignals(true);
    ui->modifierSpreadEdit->clear();
    ui->modifierSpreadEdit->blockSignals(false);

    ui->modifierDirEdit->blockSignals(true);
    ui->modifierDirEdit->clear();
    ui->modifierDirEdit->blockSignals(false);

    ui->modifierVelEdit->blockSignals(true);
    ui->modifierVelEdit->clear();
    ui->modifierVelEdit->blockSignals(false);
}

void Modifiers::updateFieldsForApplyMode()
{
    const bool isPartial = (ui->modifierApply->currentIndex() == 1);
    Modifier& mod = getBackendLayer();

    if (isPartial) {
        clearSoundFieldsForPartialMode();
    } else {
        setSoundFieldsFromBackend();
    }

    ui->modifierResEdit->blockSignals(true);
    ui->modifierResEdit->setText(isPartial ? mod.partialresult_string : "");
    ui->modifierResEdit->blockSignals(false);
}

// Keep the Apply combo box valid for the selected modifier type.
// DETUNE only supports SOUND, while other modifier types may use PARTIAL.
// This also preserves an existing PARTIAL choice when it is still allowed.
void Modifiers::updateApplyOptionsForModifierType()
{
    const int modifierType = currentModifierType(ui->modifierType);
    const bool isDetune = (modifierType == detuneType);

    Modifier& mod = getBackendLayer();
    const bool shouldUsePartial = (!isDetune && mod.applyhow_flag);

    ui->modifierApply->blockSignals(true);
    ui->modifierApply->clear();
    ui->modifierApply->addItem("SOUND");

    if (!isDetune) {
        ui->modifierApply->addItem("PARTIAL");
    }

    ui->modifierApply->setCurrentIndex(shouldUsePartial ? 1 : 0);
    ui->modifierApply->blockSignals(false);

    mod.applyhow_flag = shouldUsePartial;
}

void Modifiers::updateModState() {
    const int typeIndex = currentModifierType(ui->modifierType);

    const bool isPhaseMod = (typeIndex == phaseModType);
    ui->modifierMagLabel->setText(isPhaseMod
        ? tr("Magnitude Envelope (cycle depth):")
        : tr("Magnitude Envelope:"));
    ui->modifierRateLabel->setText(isPhaseMod
        ? tr("Rate Envelope (Hz):")
        : tr("Rate Value Envelope:"));

    // GLISSANDO (2) is always SOUND — disable the apply combo
    // In 2.1.0, GLISSANDO has enabled apply combo
    /*if (typeIndex == 2) {
        ui->modifierApply->blockSignals(true);
        ui->modifierApply->setCurrentIndex(0);
        ui->modifierApply->blockSignals(false);
        ui->modifierApply->setEnabled(false);
    } else {
        ui->modifierApply->setEnabled(true);
    }*/

    bool isPartial = (ui->modifierApply->currentIndex() == 1);

    // Enabled flags per type: { prob, mag, rate, width, spread, dir, vel, res }
    // res (partial result string) depends on apply mode, not type — set below
    //                             prob   mag    rate   width  spread dir    vel
    struct Row { QLabel* label; QLineEdit* edit; QPushButton* btn; };
    const Row rows[8] = {
        { ui->modifierProbLabel,   ui->modifierProbEdit,   ui->modifierProbFunButton   },
        { ui->modifierMagLabel,    ui->modifierMagEdit,    ui->modifierMagFunButton    },
        { ui->modifierRateLabel,   ui->modifierRateEdit,   ui->modifierRateFunButton   },
        { ui->modifierWidthLabel,  ui->modifierWidthEdit,  ui->modifierWidthFunButton  },
        { ui->modifierSpreadLabel, ui->modifierSpreadEdit, ui->modifierSpreadFunButton },
        { ui->modifierDirLabel,    ui->modifierDirEdit,    ui->modifierDirFunButton    },
        { ui->modifierVelLabel,    ui->modifierVelEdit,    ui->modifierVelFunButton    },
        { ui->modifierResLabel,    ui->modifierResEdit,    ui->modifierResFunButton    },
    };

    if (typeIndex < 0 || typeIndex >= modifierTypeCount) {
        ui->modifierApply->setEnabled(false);
        for (const Row& row : rows) {
            row.label->setEnabled(false);
            row.edit->setEnabled(false);
            row.btn->setEnabled(false);
        }
        return;
    }
    ui->modifierApply->setEnabled(true);

    for (int i = 0; i < 8; i++) {
        const bool enabled = ModifierUiPolicy::fieldEnabled(typeIndex, i, isPartial);
        rows[i].label->setEnabled(enabled);
        rows[i].edit->setEnabled(enabled);
        rows[i].btn->setEnabled(enabled);
    }
}

void Modifiers::modTextChanged(ModChanged type) {
    // Get a reference to your specific backend modifier object
    Modifier& mod = getBackendLayer();

    switch (type) {
        case modNameChanged:
            mod.group_name = ui->modifierNameEdit->text();
            break;
        case modProbabilityChanged:
            mod.probability = ui->modifierProbEdit->text();
            break;
        case modMagnitudeChanged:
            mod.amplitude = ui->modifierMagEdit->text();
            break;
        case modRateChanged:
            mod.rate = ui->modifierRateEdit->text();
            break;
        case modWidthChanged:
            mod.width = ui->modifierWidthEdit->text();
            break;
        case modPartialChanged:
            mod.partialresult_string = ui->modifierResEdit->text();
            break;
        case modSpreadChanged:
            mod.detune_spread = ui->modifierSpreadEdit->text();
            break;
        case modDirChanged:
            mod.detune_direction = ui->modifierDirEdit->text();
            break;
        case modVelChanged:
            mod.detune_velocity = ui->modifierVelEdit->text();
            break;
        default:
            break;
    }
}

void Modifiers::modFunctionButtonClicked(ModChanged type) {
    QLineEdit* target = nullptr;
    FunctionGenerator* gen = nullptr;

    switch (type) {
    case modProbabilityChanged:
        target = ui->modifierProbEdit;
        break;
    case modMagnitudeChanged:
        target = ui->modifierMagEdit;
        break;
    case modRateChanged:
        target = ui->modifierRateEdit;
        break;
    case modWidthChanged:
        target = ui->modifierWidthEdit;
        break;
    case modPartialChanged:
        target = ui->modifierResEdit;
        break;
    case modSpreadChanged:
        target = ui->modifierSpreadEdit;
        break;
    case modDirChanged:
        target = ui->modifierDirEdit;
        break;
    case modVelChanged:
        target = ui->modifierVelEdit;
        break;
    case modNameChanged:
        break;
    default:
        break;
    }

    if (!target) return;

// In PARTIAL mode, open a structured partial editor instead of the
// legacy FunctionGenerator path. PHASE_MOD uses its specialized editor;
// other non-DETUNE modifiers use the general partial editor.
const int modifierType = currentModifierType(ui->modifierType);
const bool applyByPartial = (ui->modifierApply->currentIndex() == 1);

if (type == modPartialChanged && applyByPartial) {
    ProjectManager* pm = Inst::get_project_manager();
    int spectrumPartialCount = 0;
    constexpr int generatedSpectrumPartialCount = 20;

    if (pm->get_curr_project()) {
        for (const SpectrumEvent& spectrum : pm->spectrumevents()) {
            spectrumPartialCount = std::max(
                spectrumPartialCount,
                static_cast<int>(spectrum.spectrum.partials.size()));

            bool isInteger = false;
            const int declaredCount = spectrum.num_partials.toInt(&isInteger);
            if (isInteger) {
                spectrumPartialCount = std::max(spectrumPartialCount, declaredCount);
            }

            // CMOD's generated-spectrum path currently creates 20 partials.
            if (!spectrum.generate_spectrum.trimmed().isEmpty()) {
                spectrumPartialCount = std::max(spectrumPartialCount,
                                                generatedSpectrumPartialCount);
            }
        }
    }

    if (modifierType == phaseModType) {
        PartialModifierDialog dialog(this,
                                     std::max(1, spectrumPartialCount),
                                     target->text());

        if (dialog.exec() == QDialog::Accepted) {
            target->setText(dialog.resultString());
        }

        return;
    }

    if (modifierType != 3) { // DETUNE does not support PARTIAL mode.
        GeneralPartialModifierDialog dialog(modifierType,
                                            spectrumPartialCount,
                                            target->text(),
                                            this);

        if (dialog.exec() == QDialog::Accepted) {
            const QString result = dialog.getResultString();
            if (!result.isEmpty()) {
                target->setText(result);
            }
        }

        return;
    }
}

    gen = new FunctionGenerator(nullptr, functionReturnENV, target->text());
    if (gen) {
        if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty())
            target->setText(gen->getResultString());
        delete gen;
    }
}

// Save modifier data according to the selected apply mode.
// This prevents PARTIAL mode from overwriting SOUND fields with visually cleared values.
void Modifiers::saveModifierToBackend(){
    Modifier& mod = getBackendLayer();
    saveCurrentModifierType(ui->modifierType, mod);
    mod.applyhow_flag = (ui->modifierApply->currentIndex() == 1);
    modTextChanged(modNameChanged);

    if (mod.applyhow_flag) {
        // PARTIAL mode: only PartialResultString is used.
        modTextChanged(modPartialChanged);
    } else {
        // SOUND mode: only top-level sound fields are used.
        modTextChanged(modProbabilityChanged);
        modTextChanged(modMagnitudeChanged);
        modTextChanged(modRateChanged);
        modTextChanged(modWidthChanged);
        modTextChanged(modSpreadChanged);
        modTextChanged(modDirChanged);
        modTextChanged(modVelChanged);
    }
}

void Modifiers::setModifierData(Modifier& modData) {
    ui->modifierType->blockSignals(true);
    ui->modifierType->setCurrentIndex(
        ui->modifierType->findData(static_cast<int>(modData.type)));
    ui->modifierType->blockSignals(false);

    ui->modifierApply->blockSignals(true);
    ui->modifierApply->setCurrentIndex(modData.applyhow_flag);
    ui->modifierApply->blockSignals(false);

    ui->modifierProbEdit->blockSignals(true);
    ui->modifierProbEdit->setText(modData.probability);
    ui->modifierProbEdit->blockSignals(false);

    ui->modifierMagEdit->blockSignals(true);
    ui->modifierMagEdit->setText(modData.amplitude);
    ui->modifierMagEdit->blockSignals(false);

    ui->modifierRateEdit->blockSignals(true);
    ui->modifierRateEdit->setText(modData.rate);
    ui->modifierRateEdit->blockSignals(false);

    ui->modifierWidthEdit->blockSignals(true);
    ui->modifierWidthEdit->setText(modData.width);
    ui->modifierWidthEdit->blockSignals(false);

    ui->modifierSpreadEdit->blockSignals(true);
    ui->modifierSpreadEdit->setText(modData.detune_spread);
    ui->modifierSpreadEdit->blockSignals(false);

    ui->modifierDirEdit->blockSignals(true);
    ui->modifierDirEdit->setText(modData.detune_direction);
    ui->modifierDirEdit->blockSignals(false);

    ui->modifierVelEdit->blockSignals(true);
    ui->modifierVelEdit->setText(modData.detune_velocity);
    ui->modifierVelEdit->blockSignals(false);

    ui->modifierNameEdit->blockSignals(true);
    ui->modifierNameEdit->setText(modData.group_name);
    ui->modifierNameEdit->blockSignals(false);

    ui->modifierResEdit->blockSignals(true);
    ui->modifierResEdit->setText(modData.partialresult_string);
    ui->modifierResEdit->blockSignals(false);

    updateApplyOptionsForModifierType();
    updateFieldsForApplyMode();
    updateModState();
}

Modifiers::~Modifiers()
{
    delete ui;
}


