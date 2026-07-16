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

#include <string>

#include "../ui/ui_Attributes.h"
#include "../inst.hpp"
#include "../dialogs/FunctionGenerator.hpp"
#include "../dialogs/PartialModifierDialog.hpp"

using enum FunctionReturnType;

Modifiers::Modifiers(Eventtype eventType, unsigned eventIndex, int modifierIndex, QWidget *parent)
    : QFrame(parent),
      ui(new Ui::Modifiers),
      m_eventType(eventType),
      m_eventIndex(eventIndex),
      m_modifierIndex(modifierIndex)
{
    ui->setupUi(this);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    this->setMinimumHeight(480);

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

////
int Modifiers::maxPartialCountForCurrentBottom() const
{
    if (m_eventType != bottom) {
        return 0;
    }

    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || m_eventIndex >= static_cast<unsigned>(pm->bottomevents().size())) {
        return 0;
    }

    const HEvent& bottomEvent = pm->bottomevents()[m_eventIndex].event;
    const QList<SpectrumEvent>& spectra = pm->spectrumevents();

    int maxCount = 0;

    for (const Layer& layer : bottomEvent.event_layers) {
        for (const Package& package : layer.discrete_packages) {
            bool ok = false;
            const int packageType = package.event_type.toInt(&ok);

            if (!ok || packageType != sound) {
                continue;
            }

            for (const SpectrumEvent& spectrumEvent : spectra) {
                if (spectrumEvent.name == package.event_name) {
                    maxCount = qMax(maxCount, spectrumEvent.spectrum.partials.size());
                    break;
                }
            }
        }
    }

    return maxCount;
}
////

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
            this, [this](int index) {
                getBackendLayer().type = index;
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
}


// 3 Helper Functions
void Modifiers::setSoundFieldsFromBackend() {
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

void Modifiers::clearSoundFieldsForPartialMode() {
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

void Modifiers::updateFieldsForApplyMode() {
    const bool isPartial = (ui->modifierApply->currentIndex() == 1);

    if (isPartial) {
        clearSoundFieldsForPartialMode();
    } else {
        setSoundFieldsFromBackend();
    }

    Modifier& mod = getBackendLayer();
    ui->modifierResEdit->blockSignals(true);
    ui->modifierResEdit->setText(isPartial ? mod.partialresult_string : "");
    ui->modifierResEdit->blockSignals(false);
}
////


void Modifiers::updateModState() {
    int typeIndex = ui->modifierType->currentIndex();

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
    static const bool table[7][7] = {
        /* TREMOLO  */ { true,  true,  true,  false, false, false, false },
        /* VIBRATO  */ { true,  true,  true,  false, false, false, false },
        /* GLISSANDO*/ { true,  true,  false, false, false, false, false },
        /* DETUNE   */ { true,  false, false, false, true,  true,  true  },
        /* AMPTRANS */ { true,  true,  true,  true,  false, false, false },
        /* FREQTRANS*/ { true,  true,  true,  true,  false, false, false },
        /* WAVE_TYPE*/ { true,  true,  true,  false, false, false, false },
    };

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

    bool enabled[8];
    // ISSUE#123:
    // for (int i = 0; i < 7; i++) enabled[i] = table[typeIndex][i];
    // enabled[7] = isPartial;

    // for (int i = 0; i < 8; i++) {
    //     rows[i].label->setEnabled(enabled[i]);
    //     rows[i].edit->setEnabled(enabled[i]);
    //     rows[i].btn->setEnabled(enabled[i]);
    // }
    if (isPartial) {
    // In PARTIAL mode, only Group Name and Partial Result String are editable.
    for (int i = 0; i < 7; i++) {
        enabled[i] = false;
    }
    enabled[7] = true;
    } else {
    // In SOUND mode, field availability depends on the modifier type.
    for (int i = 0; i < 7; i++) {
        enabled[i] = table[typeIndex][i];
    }
    enabled[7] = false;
    }
    ////
    for (int i = 0; i < 8; i++) {
        rows[i].label->setEnabled(enabled[i]);
        rows[i].edit->setEnabled(enabled[i]);
        rows[i].btn->setEnabled(enabled[i]);
    }

    ui->modifierNameLabel->setEnabled(true);
    ui->modifierNameEdit->setEnabled(true);
}


void Modifiers::updateApplyOptionsForModifierType()
{
    const int typeIndex = ui->modifierType->currentIndex();

    // DETUNE is index 3 in the modifier type combo.
    const bool isDetune = (typeIndex == 3);

    ui->modifierApply->blockSignals(true);

    ui->modifierApply->clear();
    ui->modifierApply->addItem("SOUND");

    if (!isDetune) {
        ui->modifierApply->addItem("PARTIAL");
    }

    if (isDetune) {
        ui->modifierApply->setCurrentIndex(0);
        getBackendLayer().applyhow_flag = false;
    } else {
        getBackendLayer().applyhow_flag = (ui->modifierApply->currentIndex() == 1);
    }

    ui->modifierApply->blockSignals(false);
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
////
    if (type == modPartialChanged
        && ui->modifierApply->currentIndex() == 1
        && ui->modifierType->currentIndex() != 3) {

        const int maxPartialCount = maxPartialCountForCurrentBottom();

        PartialModifierDialog dialog(
            ui->modifierType->currentIndex(),
            maxPartialCount,
            target->text(),
            this
        );

        if (dialog.exec() == QDialog::Accepted) {
            const QString result = dialog.getResultString();
            if (!result.isEmpty()) {
                target->setText(result);
            }
        }

        return;
    }
////
    gen = new FunctionGenerator(nullptr, functionReturnENV, target->text());
    if (gen) {
        if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty())
            target->setText(gen->getResultString());
        delete gen;
    }
}


// ISSUE#123:
void Modifiers::saveModifierToBackend() {
    Modifier& mod = getBackendLayer();

    mod.type = ui->modifierType->currentIndex();
    mod.applyhow_flag = ui->modifierApply->currentIndex();

    modTextChanged(modNameChanged);

    if (mod.applyhow_flag == 1) {
        // PARTIAL mode: sound-related fields are visually cleared and disabled,
        // so only save the partial result string.
        modTextChanged(modPartialChanged);
    } else {
        // SOUND mode: partial result string is hidden/disabled,
        // so only save sound-related fields.
        modTextChanged(modProbabilityChanged);
        modTextChanged(modMagnitudeChanged);
        modTextChanged(modRateChanged);
        modTextChanged(modWidthChanged);
        modTextChanged(modSpreadChanged);
        modTextChanged(modDirChanged);
        modTextChanged(modVelChanged);
    }
}
////

void Modifiers::setModifierData(Modifier& modData) {
    ui->modifierType->blockSignals(true);
    ui->modifierType->setCurrentIndex(modData.type);
    ui->modifierType->blockSignals(false);

    // Rebuild Apply options after setting modifier type.
    // DETUNE should only have SOUND.
    updateApplyOptionsForModifierType();

    ui->modifierApply->blockSignals(true);

    if (ui->modifierType->currentIndex() == 3) {
        // DETUNE does not support PARTIAL mode.
        ui->modifierApply->setCurrentIndex(0);
        modData.applyhow_flag = false;
    } else {
        ui->modifierApply->setCurrentIndex(modData.applyhow_flag ? 1 : 0);
    }

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

    updateFieldsForApplyMode();
    updateModState();


}

Modifiers::~Modifiers()
{
    delete ui;
}


