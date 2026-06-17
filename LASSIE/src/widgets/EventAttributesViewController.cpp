#include "EventAttributesViewController.hpp"
#include "../ui/ui_Attributes.h"

#include "../inst.hpp"
#include "../utilities.hpp"
#include "../core/event_struct.hpp"
#include "../dialogs/FunctionGenerator.hpp"
#include "ProjectViewController.hpp"
#include "../widgets/LayerBox.hpp"
#include "../widgets/Partials.hpp"
#include "../widgets/Modifiers.hpp"
#include "../ui/ui_Modifiers.h"

#include <QMessageBox>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTimer>
#include <QMetaObject>

#include <functional>

using enum FunctionReturnType;

// EventAttributesViewController::EventAttributesViewController(SharedPointers* sharedPointers,
//                                                              QWidget* parent)
EventAttributesViewController::EventAttributesViewController(ProjectView* projectView)
  : QFrame(nullptr),
    // m_soundPartialHboxes(nullptr),
    ui(new Ui::EventAttributesViewController)
{
    ui->setupUi(this);
    e_projectView = projectView;

    // --- connect child‐count mode buttons ---
    connect(ui->fixedButton, &QRadioButton::clicked,
            this, &EventAttributesViewController::fixedButtonClicked);
    connect(ui->densityButton, &QRadioButton::clicked,
            this, &EventAttributesViewController::densityButtonClicked);
    connect(ui->byLayerButton, &QRadioButton::clicked,
            this, &EventAttributesViewController::byLayerButtonClicked);

    // --- connect child‐event‐definition buttons ---
    ui->childEventDefContSweepPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->childEventDefDiscretePage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    connect(ui->continuumButton, &QRadioButton::clicked,
            this, &EventAttributesViewController::continuumButtonClicked);
    connect(ui->sweepButton, &QRadioButton::clicked,
            this, &EventAttributesViewController::sweepButtonClicked);
    connect(ui->discreteButton, &QRadioButton::clicked,
            this, &EventAttributesViewController::discreteButtonClicked);

    // --- connect "insert function" buttons ---
    connect(ui->maxChildDurFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::maxChildDurFunButtonClicked);
    connect(ui->unitsPerSecondFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::unitsPerSecondFunButtonClicked);
/*    connect(ui->unitsPerBarFunButton, &QPushButton::clicked,
            this, [=]{ insertFunctionString(unitsPerBarFunButton); });
*/   connect(ui->numOfChildFunButton1, &QPushButton::clicked,
            this, &EventAttributesViewController::numOfChildFunButton1Clicked);
    connect(ui->numOfChildFunButton2, &QPushButton::clicked,
            this, &EventAttributesViewController::numOfChildFunButton2Clicked);
    connect(ui->numOfChildFunButton3, &QPushButton::clicked,
            this, &EventAttributesViewController::numOfChildFunButton3Clicked);
    connect(ui->childEventDefStartTimeFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::childEventDefStartTimeFunButtonClicked);
    connect(ui->childEventDefChildTypeFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::childEventDefChildTypeFunButtonClicked);
    connect(ui->childEventDefDurationFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::childEventDefDurationFunButtonClicked);
    connect(ui->childEventAttackSieveFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::childEventAttackSieveButtonClicked);
    connect(ui->childEventDurationSieveFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::childEventDurationSieveButtonClicked);
    connect(ui->sieveBuilderFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::sieveBuilderButtonClicked);
    connect(ui->envBuilderFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::envBuilderFunButtonClicked);
/*    connect(ui->spectrumDeviationFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::deviationFunButtonClicked);
    connect(ui->spectrumGenerateFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::generatespectrumFunButtonClicked);

*/    // --- bottom‐sub‐attribute function buttons ---
    connect(ui->attributesStandardRevButton, &QPushButton::clicked,
            this, &EventAttributesViewController::attributesStandardRevButtonClicked);
    connect(ui->attributesStandardFilButton, &QPushButton::clicked,
            this, &EventAttributesViewController::attributesStandardFilButtonClicked);
    connect(ui->filBuilderFunButton, &QPushButton::clicked,
            this, &EventAttributesViewController::attributesFilBuilderButtonClicked);
    connect(ui->attributesStandardSpaButton, &QPushButton::clicked,
            this, &EventAttributesViewController::attributesStandardSpaButtonClicked);
    connect(ui->BSLoudnessButton, &QPushButton::clicked,
            this, &EventAttributesViewController::BSLoudnessButtonClicked);
/*    connect(ui->BSSpatializationButton, &QPushButton::clicked,
            this, &EventAttributesViewController::BSSpatializationButtonClicked);
    connect(ui->BSReverbButton, &QPushButton::clicked,
            this, &EventAttributesViewController::BSReverbButtonClicked);
    connect(ui->BSFilterButton, &QPushButton::clicked,
            this, &EventAttributesViewController::BSFilterButtonClicked);
*/    connect(ui->BSModifierGroupButton, &QPushButton::clicked,
            this, &EventAttributesViewController::BSModifierGroupButtonClicked);

    ui->BSWellTemperedPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSFundamentalPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSContinuumPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    connect(ui->wellTemperedRadio, &QRadioButton::clicked,
            this, &EventAttributesViewController::wellTemperedRadioButtonClicked);
    connect(ui->fundamentalRadio, &QRadioButton::clicked,
            this, &EventAttributesViewController::fundamentalRadioButtonClicked);
    connect(ui->continuumRadio, &QRadioButton::clicked,
            this, &EventAttributesViewController::continuumRadioButtonClicked);
          
    connect(ui->BSWellTemperedButton, &QPushButton::clicked,
            this, &EventAttributesViewController::BSWellTemperedButtonClicked);
    connect(ui->BSFunFreqButton1, &QPushButton::clicked,
            this, &EventAttributesViewController::BSFunFreqButton1Clicked);
    connect(ui->BSFunFreqButton2, &QPushButton::clicked,
            this, &EventAttributesViewController::BSFunFreqButton2Clicked);
    connect(ui->BSContinuumButton, &QPushButton::clicked,
            this, &EventAttributesViewController::BSContinuumButtonClicked);


    // --- additional controls --

    connect(ui->addNewLayerButton, &QPushButton::clicked,
            this, &EventAttributesViewController::addNewLayerButtonClicked);
    connect(ui->addModifierButton, &QPushButton::clicked,
            this, &EventAttributesViewController::addModifierButtonClicked);
    connect(ui->addPartialButton, &QPushButton::clicked,
            this, &EventAttributesViewController::addPartialButtonClicked);

    // --- tempo controls ---
    ui->tempoValuePage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->tempoFractionPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    connect(ui->tempoAsNoteValueRadio, &QRadioButton::clicked,
            this, &EventAttributesViewController::tempoAsNoteValueButtonClicked);
    connect(ui->tempoAsFractionRadio, &QRadioButton::clicked,
            this, &EventAttributesViewController::tempoAsFractionButtonClicked);
/*    connect(ui->tempoValueEntry, &QLineEdit::textChanged,
            this, &EventAttributesViewController::tempoAsNoteValueEntryChanged);
    connect(ui->tempoFractionEntry1, &QLineEdit::textChanged,
            this, &EventAttributesViewController::tempoAsNoteValueEntryChanged);
*/
    setFocusPolicy(Qt::StrongFocus);
    ui->stackedWidget->setCurrentWidget(ui->emptyPage);
    fixStackedWidgetLayout(ui->emptyPage);

    ui->layersLayout->setSpacing(8);
    ui->layersLayout->setContentsMargins(0, 0, 0, 0);
}

EventAttributesViewController::~EventAttributesViewController() = default;

void EventAttributesViewController::fixStackedWidgetLayout(QWidget* currPage) {
    if (!currPage) return;
    
    // Keep inactive pages from inflating the stacked widget's sizeHint.
    QList<QWidget*> pages = {
        ui->emptyPage,
        ui->standardPage,
        ui->soundPage,
        ui->envPage,
        ui->sievePage,
        ui->spaPage,
        ui->patPage,
        ui->revPage,
        ui->filPage,
        ui->meaPage,
        ui->notePage
    };
    for (QWidget* page : pages) {
        if (page == currPage) {
            // Allow the current page to dictate its own size
            page->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        } else {
            // Keep inactive pages from inflating the container
            page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        }
    }

    if (currPage->layout()) {
        currPage->layout()->activate(); 
        currPage->layout()->invalidate(); 
    }

    currPage->adjustSize();

    if (currPage == ui->soundPage && ui->soundPage->layout()) {
        ui->soundPage->layout()->setSpacing(10);
        ui->partialsLayout->setSpacing(0);
    }
    if (currPage == ui->standardPage && ui->standardPage->layout()) {
        ui->standardPage->layout()->setSpacing(10);
        ui->modifiersLayout->setSpacing(0);
    }

    // Notify the parent scroll area that our sizeHint has changed so it
    // re-evaluates the widget size rather than relying on the stale value.
    // Subjective: this seems to positively impact the appearance when you add
    // LayerBoxes. -Jacob, 3/9/26
    updateGeometry();
}

void EventAttributesViewController::updateNameEntryIfShowing(const QString& typeStr, int index, const QString& name)
{
    static const QMap<QString, Eventtype> typeMap = {
        {"Top", top}, {"High", high}, {"Mid", mid}, {"Low", low}, {"Bottom", bottom},
        {"Spectrum", sound}, {"Note", note}, {"Envelope", env}, {"Sieve", sieve},
        {"Spatialization", spa}, {"Pattern", pattern}, {"Reverb", reverb}, {"Filter", filter}
    };
    if (!typeMap.contains(typeStr)) return;
    Eventtype et = typeMap[typeStr];
    if (!m_hasCurrentEvent || m_curreventtype != et || m_curreventindex != index) return;

    if (et <= bottom)         ui->nameEntry->setText(name);
    else if (et == sound)     ui->soundNameEntry->setText(name);
    else if (et == env)       ui->envNameEntry->setText(name);
    else if (et == sieve)     ui->sieveNameEntry->setText(name);
    else if (et == spa)       ui->spaNameEntry->setText(name);
    else if (et == pattern)   ui->patNameEntry->setText(name);
    else if (et == reverb)    ui->revNameEntry->setText(name);
    else if (et == note)      ui->noteNameEntry->setText(name);
    else if (et == filter)    ui->filNameEntry->setText(name);
}

void EventAttributesViewController::reloadAllLayerBoxes() {
    for (LayerBox* box : m_layerBoxes) {
        if (box) box->reloadFromBackend();
    }
}

void EventAttributesViewController::showAttributesOfEvent(Eventtype type, int index) {
    if (m_hasCurrentEvent && (m_curreventtype != type || m_curreventindex != index)) {
        saveCurrentShownEventData();
        // m_modifiers.clear();
    }
    m_curreventtype = type;
    m_curreventindex = index;
    m_hasCurrentEvent = true;

    showCurrentEventData();
}

void EventAttributesViewController::clearView() {
    for (LayerBox* box : m_layerBoxes) {
        ui->layersLayout->removeWidget(box);
        box->deleteLater();
    }
    m_layerBoxes.clear();

    ui->stackedWidget->setCurrentWidget(ui->emptyPage);
    fixStackedWidgetLayout(ui->emptyPage);
    m_hasCurrentEvent = false;
}

void EventAttributesViewController::onEventDeleted(Eventtype type, int deletedIndex) {
    if (!m_hasCurrentEvent || m_curreventtype != type) return;

    if (m_curreventindex == deletedIndex) {
        clearView();
    } else if (m_curreventindex > deletedIndex) {
        --m_curreventindex;
        // Keep the LayerBox widgets pointing at the right (now-shifted) event
        for (LayerBox* box : m_layerBoxes) {
            box->setEventIndex(m_curreventindex);
        }
    }
}

void EventAttributesViewController::saveCurrentShownEventData() {
    if(m_curreventindex == -1) return;
    //qDebug() << "saveCurrentShownEventData called on type" << (int)m_curreventtype << "of index" << m_curreventindex;
    
    if (!m_hasCurrentEvent) return;

    qDebug() << "saveCurrentShownEventData called";

    Eventtype type = m_curreventtype;
    ProjectManager *pm = Inst::get_project_manager();

    // Reject if the new name collides with an existing event of the same type.
    {
        QString newName;
        bool isDuplicate = false;

        auto checkList = [&](const auto& list, const QString& name) {
            for (int i = 0; i < list.size(); ++i) {
                if (i != m_curreventindex && list[i].name == name) return true;
            }
            return false;
        };

        if (type == high || type == mid || type == low) {
            newName = ui->nameEntry->text();
            QList<HEvent>& list = (type == high) ? pm->highevents()
                                : (type == mid)  ? pm->midevents()
                                                 : pm->lowevents();
            isDuplicate = checkList(list, newName);
        } else if (type == bottom) {
            newName = ui->nameEntry->text();
            const auto& list = pm->bottomevents();
            for (int i = 0; i < list.size(); ++i) {
                if (i != m_curreventindex && list[i].event.name == newName) { isDuplicate = true; break; }
            }
        } else if (type == sound) {
            newName = ui->soundNameEntry->text();
            isDuplicate = checkList(pm->spectrumevents(), newName);
        } else if (type == env) {
            newName = ui->envNameEntry->text();
            isDuplicate = checkList(pm->envelopeevents(), newName);
        } else if (type == sieve) {
            newName = ui->sieveNameEntry->text();
            isDuplicate = checkList(pm->sieveevents(), newName);
        } else if (type == spa) {
            newName = ui->spaNameEntry->text();
            isDuplicate = checkList(pm->spaevents(), newName);
        } else if (type == pattern) {
            newName = ui->patNameEntry->text();
            isDuplicate = checkList(pm->patternevents(), newName);
        } else if (type == reverb) {
            newName = ui->revNameEntry->text();
            isDuplicate = checkList(pm->reverbevents(), newName);
        } else if (type == note) {
            newName = ui->noteNameEntry->text();
            isDuplicate = checkList(pm->noteevents(), newName);
        } else if (type == filter) {
            newName = ui->filNameEntry->text();
            isDuplicate = checkList(pm->filterevents(), newName);
        }

        if (isDuplicate) {
            QMessageBox::warning(this, "Duplicate Name",
                QString("An event named \"%1\" already exists for this type. Please choose a different name.")
                    .arg(newName));
            return;
        }

	if (type == bottom && !newName.isEmpty() &&
            newName[0] != QLatin1Char('s') && newName[0] != QLatin1Char('n')) {
            QMessageBox::warning(this, "Invalid Name",
                "Bottom Event names must start with 's' or 'n'.");
            return;
        }
    }

    if (type == bottom) {
        BottomEvent& bottom_event = pm->bottomevents()[m_curreventindex];
        ExtraInfo& extra_info = bottom_event.extra_info;

        FreqInfo& freq_info = extra_info.freq_info;
        if (ui->wellTemperedRadio->isChecked()) {
            freq_info.freq_flag = 0;
            freq_info.entry_1 = ui->wellTemperedEntry->text();
        } else if (ui->fundamentalRadio->isChecked()) {
            freq_info.freq_flag = 1;
            freq_info.entry_1 = ui->funFreqEntry1->text();
            freq_info.entry_2 = ui->funFreqEntry2->text();
        } else if (ui->continuumRadio->isChecked()) {
            freq_info.freq_flag = 2;
            freq_info.entry_1 = ui->continuumFreqEntry->text();
        }
        if (ui->hertzRadio->isChecked()) { freq_info.continuum_flag = 0; }
        if (ui->powerOfTwoRadio->isChecked()) { freq_info.continuum_flag = 1; }

        extra_info.loudness = ui->loudnessEntry->text();
        extra_info.spa = ui->spaEntry->text();
        extra_info.reverb = ui->revEntry->text();
        extra_info.filter = ui->filEntry->text();
        extra_info.modifier_group = ui->modifierGroupEntry->text();
        
    }

    if(type <= bottom){
        HEvent& event = [&]() -> HEvent& {
            if(type == top)
                return pm->topevent();
            if(type == high)
                return pm->highevents()[m_curreventindex];
            if(type == mid)
                return pm->midevents()[m_curreventindex];
            if(type == low)
                return pm->lowevents()[m_curreventindex];
            return pm->bottomevents()[m_curreventindex].event;
        }();

        event.name = ui->nameEntry->text();

        event.max_child_duration = ui->maxChildDurEntry->text();
        event.timesig.bar_value = ui->timeSig1Entry->text();
        event.timesig.note_value = ui->timeSig2Entry->text();
        event.edu_perbeat = ui->unitsPerSecondEntry->text();

        Tempo& temp = event.tempo;
        if (ui->tempoAsNoteValueRadio->isChecked()) { /* as note */
            temp.method_flag = 0;
            temp.prefix = QString::number(ui->tempoNotePrefixCombo1->currentIndex());

            temp.note_value = QString::number(ui->tempoNoteCombo1->currentIndex());
            temp.valentry = ui->tempoValueEntry->text();
        } else { /* as note */
            temp.frentry_1 = ui->tempoFractionEntry1->text();
            temp.frentry_2 = ui->tempoFractionEntry2->text();
            temp.prefix = QString::number(ui->tempoNotePrefixCombo2->currentIndex());
            temp.note_value = QString::number(ui->tempoNoteCombo2->currentIndex());
        }  

        NumChildren& num_children = event.numchildren;
        if (ui->fixedButton->isChecked()) {
            num_children.method_flag = 0;
        } else if (ui->densityButton->isChecked()) {
            num_children.method_flag = 1;
        } else if (ui->byLayerButton->isChecked()) {
            num_children.method_flag = 2;
        }  
        num_children.entry_1 = ui->childCountEntry1->text();
        num_children.entry_2 = ui->childCountEntry2->text();
        num_children.entry_3 = ui->childCountEntry3->text();
        
        ChildDef& childeventdef = event.child_event_def;
        if (ui->discreteButton->isChecked()) {
            childeventdef.definition_flag = 2;
        } else if (ui->continuumButton->isChecked()) {
            childeventdef.definition_flag = 0;
        } else if (ui->sweepButton->isChecked()) {
            childeventdef.definition_flag = 1;
        }
        childeventdef.entry_1 = ui->childDefEntry1->text();
        childeventdef.entry_2 = ui->childDefEntry2->text();
        childeventdef.entry_3 = ui->childDefEntry3->text();
        childeventdef.attack_sieve = ui->attackSieveEntry->text();
        childeventdef.duration_sieve = ui->durationSieveEntry->text();

        if (ui->startTypePercentRadio->isChecked()) {
            childeventdef.starttype_flag = 0;
        } else if (ui->startTypeUnitsRadio->isChecked()) {
            childeventdef.starttype_flag = 1;
        } else if (ui->startTypeSecondsRadio->isChecked()) {
            childeventdef.starttype_flag = 2;
        }

        if (ui->durationTypePercentRadio->isChecked()) {
            childeventdef.durationtype_flag = 0;
        } else if (ui->durationTypeUnitsRadio->isChecked()) {
            childeventdef.durationtype_flag = 1;
        } else if (ui->durationTypeSecondsRadio->isChecked()) {
            childeventdef.durationtype_flag = 2;
        }

        if (type != bottom) {
            event.spa = ui->spaEntry->text();
            event.reverb = ui->revEntry->text();
            event.filter = ui->filEntry->text();
        }

        // save modifiers
        for (Modifiers* mod : m_modifiers) {
            mod->saveModifierToBackend();
        }

        // save layer weights
        for (LayerBox* box : m_layerBoxes) {
            box->saveWeightToBackend();
        }
    } else {

        if (type == sound) {

            SpectrumEvent& event = pm->spectrumevents()[m_curreventindex];
            event.name = ui->soundNameEntry->text();
            event.num_partials = ui->spectrumNumPartialEntry->text();
            event.deviation = ui->spectrumDeviationEntry->text();
            event.generate_spectrum = ui->spectrumGenEntry->text();

            qDebug() << "saving spectrum partials";
            for (Partials* par : m_partials) {
                par->saveWeightToBackend();
            }
        }
        if (type == env){
            EnvelopeEvent& event = pm->envelopeevents()[m_curreventindex];
            event.name = ui->envNameEntry->text();
            event.envelope_builder = ui->envBuilderEntry->text();
        } else if (type == sieve) {
            SieveEvent& event = pm->sieveevents()[m_curreventindex];
            event.name = ui->sieveNameEntry->text();
            event.sieve_builder = ui->sieveBuilderEntry->text();
        } else if (type == spa) {
            SpaEvent& event = pm->spaevents()[m_curreventindex];
            event.name = ui->spaNameEntry->text();
            event.spatialization = ui->spaBuilderEntry->text();
        } else if (type == pattern) {
            PatternEvent& event = pm->patternevents()[m_curreventindex];
            event.name = ui->patNameEntry->text();
            event.pattern_builder = ui->patBuilderEntry->text();
        } else if (type == reverb) {
            ReverbEvent& event = pm->reverbevents()[m_curreventindex];
            event.name = ui->revNameEntry->text();
            event.reverberation = ui->revBuilderEntry->text();
        } else if (type == note) {
            NoteEvent& event = pm->noteevents()[m_curreventindex];
            event.name = ui->noteNameEntry->text();
            event.note_info.staffs = ui->staffNumberEntry->text();
        } else if (type == filter) {
            FilterEvent& event = pm->filterevents()[m_curreventindex];
            event.name = ui->filNameEntry->text();
            event.filter_builder = ui->filBuilderEntry->text();
        } else {
            //qDebug() << "Cannot save event data: type of event not valid";
        }

    }

    // Sync the saved name back to the palette
    QString typeStr;
    QString savedName;
    switch (m_curreventtype) {
        case top:     /* Top name is fixed — never sync to palette */                    break;
        case high:    typeStr = "High";           savedName = ui->nameEntry->text();      break;
        case mid:     typeStr = "Mid";            savedName = ui->nameEntry->text();      break;
        case low:     typeStr = "Low";            savedName = ui->nameEntry->text();      break;
        case bottom:  typeStr = "Bottom";         savedName = ui->nameEntry->text();      break;
        case sound:   typeStr = "Spectrum";       savedName = ui->soundNameEntry->text(); break;
        case env:     typeStr = "Envelope";       savedName = ui->envNameEntry->text();   break;
        case sieve:   typeStr = "Sieve";          savedName = ui->sieveNameEntry->text(); break;
        case spa:     typeStr = "Spatialization"; savedName = ui->spaNameEntry->text();   break;
        case pattern: typeStr = "Pattern";        savedName = ui->patNameEntry->text();   break;
        case reverb:  typeStr = "Reverb";         savedName = ui->revNameEntry->text();   break;
        case note:    typeStr = "Note";           savedName = ui->noteNameEntry->text();  break;
        case filter:  typeStr = "Filter";         savedName = ui->filNameEntry->text();   break;
        default: break;
    }
    if (!typeStr.isEmpty())
        e_projectView->updatePaletteItemName(typeStr, m_curreventindex, savedName);

    MUtilities::modified();
}

void EventAttributesViewController::showCurrentEventData() {

    qDebug() << "showCurrentEventData() called";

    // Clear all existing LayerBox widgets so the panel reflects the new event
    for (LayerBox* box : m_layerBoxes) {
        ui->layersLayout->removeWidget(box);
        box->deleteLater();
    }
    m_layerBoxes.clear();

    // Choose page based on type of currently shown event
    Eventtype type = m_curreventtype;
    //qDebug() << "In showCurrentEventData saving " << type;
    switch(type) {
        case top: 
        case high: 
        case mid: 
        case low: 
        case bottom:
            ui->stackedWidget->setCurrentWidget(ui->standardPage);
            if (type == bottom) {  
                ui->frequencyContainer->setVisible(true);
                ui->loudnessContainer->setVisible(true);
                ui->modGroupContainer->setVisible(true);
            } else {
                ui->frequencyContainer->setVisible(false);
                ui->loudnessContainer->setVisible(false);
                ui->modGroupContainer->setVisible(false);
            }
            fixStackedWidgetLayout(ui->standardPage);
            break;
        case sound:
            ui->stackedWidget->setCurrentWidget(ui->soundPage);
            break;
        case env:
            ui->stackedWidget->setCurrentWidget(ui->envPage);
            fixStackedWidgetLayout(ui->envPage);
            break;
        case sieve:
            ui->stackedWidget->setCurrentWidget(ui->sievePage);
            fixStackedWidgetLayout(ui->sievePage);
            break;
        case spa:
            ui->stackedWidget->setCurrentWidget(ui->spaPage);
            fixStackedWidgetLayout(ui->spaPage);
            break;
        case pattern:
            ui->stackedWidget->setCurrentWidget(ui->patPage);
            fixStackedWidgetLayout(ui->patPage);
            break;
        case reverb:
            ui->stackedWidget->setCurrentWidget(ui->revPage);     
            fixStackedWidgetLayout(ui->revPage);  
            break;
        case filter:
            ui->stackedWidget->setCurrentWidget(ui->filPage);
            fixStackedWidgetLayout(ui->filPage);
            break;
        case mea:
            ui->stackedWidget->setCurrentWidget(ui->meaPage);
            fixStackedWidgetLayout(ui->meaPage);
            break;
        case note:
            ui->stackedWidget->setCurrentWidget(ui->notePage);
            fixStackedWidgetLayout(ui->notePage);
            break;
        default:
            ui->stackedWidget->setCurrentWidget(ui->emptyPage);
            fixStackedWidgetLayout(ui->emptyPage);
    }

    // populate fields
    ProjectManager *pm = Inst::get_project_manager();
    // ui->nameEntry->setText(QString::fromStdString(m_currentlyShownEvent->getEventName()));
    HEvent event;
    if(type <= bottom){
        if(type == bottom){
            const BottomEvent& bottom_event = pm->bottomevents()[m_curreventindex];
            ExtraInfo extra_info = bottom_event.extra_info;

            FreqInfo freq_info = extra_info.freq_info;
            ui->wellTemperedRadio->setChecked(freq_info.freq_flag == 0);
            ui->fundamentalRadio->setChecked(freq_info.freq_flag == 1);
            ui->continuumRadio->setChecked(freq_info.freq_flag == 2);
            if (freq_info.freq_flag == 0) wellTemperedRadioButtonClicked();
            else if (freq_info.freq_flag == 1) fundamentalRadioButtonClicked();
            else if (freq_info.freq_flag == 2) continuumRadioButtonClicked();
            ui->wellTemperedEntry->setText(freq_info.entry_1);
            ui->funFreqEntry1->setText(freq_info.entry_1);
            ui->funFreqEntry2->setText(freq_info.entry_2);
            ui->continuumFreqEntry->setText(freq_info.entry_1);
            ui->hertzRadio->setChecked(freq_info.continuum_flag == 0);
            ui->powerOfTwoRadio->setChecked(freq_info.continuum_flag == 1);

            ui->loudnessEntry->setText(extra_info.loudness);
            ui->spaEntry->setText(extra_info.spa);
            ui->revEntry->setText(extra_info.reverb);
            ui->filEntry->setText(extra_info.filter);
            ui->modifierGroupEntry->setText(extra_info.modifier_group);

            // clear existing Modifiers widgets
            for (Modifiers* mod : m_modifiers) {
                ui->modifiersLayout->removeWidget(mod);
                mod->deleteLater();
            }
            m_modifiers.clear();

            // rebuild buttom Modifiers
            for (int i = 0; i < extra_info.modifiers.size(); ++i) {
                addModifiersUI(i);
                m_modifiers[i]->setModifierData(extra_info.modifiers[i]);
            }
            
            event = bottom_event.event;
        }else if(type == top){
            event = pm->topevent();
        }else if(type == high){
            event = pm->highevents()[m_curreventindex];
        }else if(type == mid){
            event = pm->midevents()[m_curreventindex];
        }else{
            event = pm->lowevents()[m_curreventindex];
        }
        ui->nameEntry->setText(event.name);
        ui->nameEntry->setEnabled(false); //gray out the event

        ui->maxChildDurEntry->setText(event.max_child_duration);
        ui->timeSig1Entry->setText(event.timesig.bar_value);
        ui->timeSig2Entry->setText(event.timesig.note_value);
        ui->unitsPerSecondEntry->setText(event.edu_perbeat);

        // tempo
        Tempo tempo = event.tempo;
        /// \todo sort this out
        // ui->tempoNotePrefixCombo->setCurrentIndex(static_cast<int>(tempo.prefix));
        // ui->tempoNoteValueCombo->setCurrentIndex(static_cast<int>(tempo.note_value));
        
        if(tempo.method_flag == 0){  /* as note */
            ui->tempoAsNoteValueRadio->click();
            ui->tempoNotePrefixCombo1->setCurrentIndex(tempo.prefix.toUInt());
            ui->tempoNoteCombo1->setCurrentIndex(tempo.note_value.toUInt());
            ui->tempoValueEntry->setText(tempo.valentry);
        }else{ /* as note */
            ui->tempoAsFractionRadio->click();
            ui->tempoFractionEntry1->setText(tempo.frentry_1);
            ui->tempoFractionEntry2->setText(tempo.frentry_2);
            ui->tempoNotePrefixCombo2->setCurrentIndex(tempo.prefix.toUInt());
            ui->tempoNoteCombo2->setCurrentIndex(tempo.note_value.toUInt());
        }

        // num children
        NumChildren num_children = event.numchildren;
        switch(num_children.method_flag){
            case 0:
                ui->fixedButton->click();
                break;
            case 1:
                ui->densityButton->click();
                break;
            case 2:
                ui->byLayerButton->click();
        }
        ui->childCountEntry1->setText(num_children.entry_1);
        ui->childCountEntry2->setText(num_children.entry_2);
        ui->childCountEntry3->setText(num_children.entry_3);

        // child‐event‐def
        ChildDef childeventdef = event.child_event_def;
        switch(childeventdef.definition_flag){
            case 2:
                ui->discreteButton->click();
                break;
            case 0:
                ui->continuumButton->click();
                break;
            case 1:
                ui->sweepButton->click();
                break;

        }
        ui->childDefEntry1->setText(childeventdef.entry_1);
        ui->childDefEntry2->setText(childeventdef.entry_2);
        ui->childDefEntry3->setText(childeventdef.entry_3);
        ui->attackSieveEntry->setText(childeventdef.attack_sieve);
        ui->durationSieveEntry->setText(childeventdef.duration_sieve);

        unsigned st_flag = childeventdef.starttype_flag;
        ui->startTypePercentRadio->setChecked(st_flag == 0);
        ui->startTypeUnitsRadio->setChecked(st_flag == 1);
        ui->startTypeSecondsRadio->setChecked(st_flag == 2);

        unsigned dt_flag = childeventdef.durationtype_flag;
        ui->durationTypePercentRadio->setChecked(dt_flag == 0);
        ui->durationTypeUnitsRadio->setChecked(dt_flag == 1);
        ui->durationTypeSecondsRadio->setChecked(dt_flag == 2);

        // clear and rebuild hevent modifier widgets
        if (type != bottom) {
            // clear existing Modifiers widgets
            for (Modifiers* mod : m_modifiers) {
                ui->modifiersLayout->removeWidget(mod);
                mod->deleteLater();
            }
            m_modifiers.clear();

            // rebuild Modifiers
            for (int i = 0; i < event.modifiers.size(); ++i) {
                addModifiersUI(i);
                m_modifiers[i]->setModifierData(event.modifiers[i]);
            }
 
        }
        // environment
        if (type != bottom) {
            ui->spaEntry->setText(event.spa);
            ui->revEntry->setText(event.reverb);
            ui->filEntry->setText(event.filter);
        }

        // Rebuild LayerBoxes for the newly shown event
        for (int i = 0; i < event.event_layers.size(); ++i) {
            addLayerBoxUI(i);
        }
    }else{
        if (type == sound) {
            const SpectrumEvent& event = pm->spectrumevents()[m_curreventindex];
            ui->soundNameEntry->setText(event.name);
            ui->soundNameEntry->setEnabled(false);
            ui->spectrumNumPartialEntry->setText(event.num_partials);
            ui->spectrumNumPartialEntry->setEnabled(false); //gray out number of partials
            ui->spectrumDeviationEntry->setText(event.deviation);
            ui->spectrumGenEntry->setText(event.generate_spectrum);

            // Clear all existing Partials widgets so the panel reflects the new event
            for (Partials* par : m_partials) {
                ui->partialsLayout->removeWidget(par);
                par->deleteLater();
            }
            m_partials.clear();

            // Rebuild spectrum partials
            for (int i = 0; i < event.spectrum.partials.size(); ++i) {
                addPartialsUI(i);
                m_partials[i]->setPartialText(event.spectrum.partials[i]);
            }
        }else if(type == env){
            const EnvelopeEvent& event = pm->envelopeevents()[m_curreventindex];
            ui->envNameEntry->setText(event.name);
            ui->envNameEntry->setEnabled(false);
            ui->envBuilderEntry->setText(event.envelope_builder);
        }else if(type == sieve){
            const SieveEvent& event = pm->sieveevents()[m_curreventindex];
            ui->sieveNameEntry->setText(event.name);
            ui->sieveNameEntry->setEnabled(false);
            ui->sieveBuilderEntry->setText(event.sieve_builder);
        }else if(type == spa){
            const SpaEvent& event = pm->spaevents()[m_curreventindex];
            ui->spaNameEntry->setText(event.name);
            ui->spaNameEntry->setEnabled(false);
            ui->spaBuilderEntry->setText(event.spatialization);
        }else if(type == pattern){
            const PatternEvent& event = pm->patternevents()[m_curreventindex];
            ui->patNameEntry->setText(event.name);
            ui->patNameEntry->setEnabled(false);
            ui->patBuilderEntry->setText(event.pattern_builder);
        }else if(type == reverb){
            const ReverbEvent& event = pm->reverbevents()[m_curreventindex];
            ui->revNameEntry->setText(event.name);
            ui->revNameEntry->setEnabled(false);
            ui->revBuilderEntry->setText(event.reverberation);
        }else if(type == note){
            const NoteEvent& event = pm->noteevents()[m_curreventindex];
            ui->noteNameEntry->setText(event.name);
            ui->noteNameEntry->setEnabled(false);
            ui->staffNumberEntry->setText(event.note_info.staffs);
        }else if(type == filter){
            const FilterEvent& event = pm->filterevents()[m_curreventindex];
            ui->filNameEntry->setText(event.name);
            ui->filNameEntry->setEnabled(false);
            ui->filBuilderEntry->setText(event.filter_builder);
        }else{
            qDebug() << "Cannot show event data: type of event not valid";
        }
    } /* not hevent nor bottom event */
}

void EventAttributesViewController::fixedButtonClicked() {
    densityButtonClicked();
    ui->numOfChildLabel1->setText("Number of Children To Create:");
    ui->numOfChildLabel2->setText("");
    ui->numOfChildLabel3->setText("");
    ui->childCountEntry2->setEnabled(false);
    ui->numOfChildFunButton2->setEnabled(false);
    ui->childCountEntry3->setEnabled(false);
    ui->numOfChildFunButton3->setEnabled(false);
    for (auto* box : m_layerBoxes) box->setWeightEnabled(false);
}

void EventAttributesViewController::densityButtonClicked() {
    ui->numOfChildLabel1->setText("Density:");
    ui->numOfChildLabel2->setText("Area:");
    ui->numOfChildLabel3->setText("Under One:");
    ui->childCountEntry1->setEnabled(true);
    ui->numOfChildFunButton1->setEnabled(true);
    ui->childCountEntry2->setEnabled(true);
    ui->numOfChildFunButton2->setEnabled(true);
    ui->childCountEntry3->setEnabled(true);
    ui->numOfChildFunButton3->setEnabled(true);
    for (auto* box : m_layerBoxes) box->setWeightEnabled(false);
}

void EventAttributesViewController::byLayerButtonClicked() {
    ui->numOfChildLabel1->setText("");
    ui->numOfChildLabel2->setText("");
    ui->numOfChildLabel3->setText("");
    ui->childCountEntry1->setEnabled(false);
    ui->numOfChildFunButton1->setEnabled(false);
    ui->childCountEntry2->setEnabled(false);
    ui->numOfChildFunButton2->setEnabled(false);
    ui->childCountEntry3->setEnabled(false);
    ui->numOfChildFunButton3->setEnabled(false);
    for (auto* box : m_layerBoxes) box->setWeightEnabled(true);
}

void EventAttributesViewController::continuumButtonClicked() {
    ui->childEventDefStack->setCurrentWidget(ui->childEventDefContSweepPage);
    ui->emptyChildEventDefPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->childEventDefDiscretePage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->childEventDefContSweepPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->childEventDefStack->updateGeometry();
    for (LayerBox* box : m_layerBoxes) box->setPackageFieldsVisible(false);
}

void EventAttributesViewController::sweepButtonClicked() {
    continuumButtonClicked();
}

void EventAttributesViewController::discreteButtonClicked() {
    ui->childEventDefStack->setCurrentWidget(ui->childEventDefDiscretePage);
    ui->emptyChildEventDefPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->childEventDefContSweepPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->childEventDefDiscretePage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->childEventDefStack->updateGeometry();
    // ui->childDefEntry1->setEnabled(false);
    // ui->childDefEntry2->setEnabled(false);
    // ui->childDefEntry3->setEnabled(false);
    // ui->attackSieveEntry->setEnabled(true);
    // ui->durationSieveEntry->setEnabled(true);
    // ui->startTypePercentRadio->setEnabled(false);
    // ui->startTypeUnitsRadio->setEnabled(false);
    // ui->startTypeSecondsRadio->setEnabled(false);
    // ui->durationTypePercentRadio->setEnabled(false);
    // ui->durationTypeUnitsRadio->setEnabled(false);
    // ui->durationTypeSecondsRadio->setEnabled(false);
    for (LayerBox* box : m_layerBoxes) box->setPackageFieldsVisible(true);
}

void EventAttributesViewController::wellTemperedRadioButtonClicked() {
    ui->frequencyStack->setCurrentWidget(ui->BSWellTemperedPage);
    ui->emptyFrequencyPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSFundamentalPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSContinuumPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    ui->BSWellTemperedPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->frequencyStack->updateGeometry();
}

void EventAttributesViewController::fundamentalRadioButtonClicked() {
    ui->frequencyStack->setCurrentWidget(ui->BSFundamentalPage);
    ui->emptyFrequencyPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSWellTemperedPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSContinuumPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    ui->BSFundamentalPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->frequencyStack->updateGeometry();
}

void EventAttributesViewController::continuumRadioButtonClicked() {
    ui->frequencyStack->setCurrentWidget(ui->BSContinuumPage);
    ui->emptyFrequencyPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSWellTemperedPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->BSFundamentalPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    ui->BSContinuumPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->frequencyStack->updateGeometry();
}

void EventAttributesViewController::maxChildDurFunButtonClicked() {
    insertFunctionString(maxChildDurFunButton);
}

void EventAttributesViewController::unitsPerSecondFunButtonClicked() {
    insertFunctionString(unitsPerSecondFunButton);
}

void EventAttributesViewController::numOfChildFunButton1Clicked() {
    insertFunctionString(numOfChildFunButton1);
}

void EventAttributesViewController::numOfChildFunButton2Clicked() {
    insertFunctionString(numOfChildFunButton2);
}

void EventAttributesViewController::numOfChildFunButton3Clicked() {
    insertFunctionString(numOfChildFunButton3);
}

void EventAttributesViewController::childEventDefStartTimeFunButtonClicked() {
    insertFunctionString(childEventDefStartTimeFunButton);
}

void EventAttributesViewController::childEventDefChildTypeFunButtonClicked() {
    insertFunctionString(childEventDefChildTypeFunButton);
}

void EventAttributesViewController::childEventDefDurationFunButtonClicked() {
    insertFunctionString(childEventDefDurationFunButton);
}

void EventAttributesViewController::childEventAttackSieveButtonClicked() {
    insertFunctionString(childEventDefAttackSieveFunButton);
}

void EventAttributesViewController::childEventDurationSieveButtonClicked() {
    insertFunctionString(childEventDefDurationSieveFunButton);
}

void EventAttributesViewController::sieveBuilderButtonClicked() {
    insertFunctionString(sieveBuilderFunButton);
}

void EventAttributesViewController::envBuilderFunButtonClicked() {
    insertFunctionString(envBuilderFunButton);
}
/*
void EventAttributesViewController::deviationFunButtonClicked() {
    insertFunctionString(spectrumDeviationFunButton);
}

void EventAttributesViewController::generatespectrumFunButtonClicked() {
    insertFunctionString(spectrumGenerateFunButton);
}
*/
void EventAttributesViewController::attributesStandardRevButtonClicked() {
    insertFunctionString(attributesRevFunButton);
}

void EventAttributesViewController::attributesStandardFilButtonClicked() {
    insertFunctionString(attributesFilFunButton);
}

void EventAttributesViewController::attributesFilBuilderButtonClicked() {
    insertFunctionString(attributesFilBuilderFunButton);
}

void EventAttributesViewController::attributesStandardSpaButtonClicked() {
    insertFunctionString(attributesSpaFunButton);
}

void EventAttributesViewController::BSLoudnessButtonClicked() {
    insertFunctionString(BSLoudnessFunButton);
}
/*
void EventAttributesViewController::BSSpatializationButtonClicked() {
    if (m_currentlyShownEvent->getEventExtraInfo()->getChildTypeFlag() != 0) return;
    insertFunctionString(spectrumDeviationFunButton);
}

void EventAttributesViewController::BSReverbButtonClicked() {
    if (m_currentlyShownEvent->getEventExtraInfo()->getChildTypeFlag() != 0) return;
    insertFunctionString(spectrumGenerateFunButton);
}

void EventAttributesViewController::BSFilterButtonClicked() {
    if (m_currentlyShownEvent->getEventExtraInfo()->getChildTypeFlag() != 0) return;
    insertFunctionString(spectrumGenerateFunButton);
}
*/
void EventAttributesViewController::BSModifierGroupButtonClicked() {
    // if (m_currentlyShownEvent->getEventExtraInfo()->getChildTypeFlag() != 0) return;
    insertFunctionString(BSModGroupFunButton);
}

void EventAttributesViewController::BSWellTemperedButtonClicked() {
    insertFunctionString(BSWellTemperedFunButton);
}

void EventAttributesViewController::BSFunFreqButton1Clicked() {
    insertFunctionString(BSFunFreq1FunButton);
}

void EventAttributesViewController::BSFunFreqButton2Clicked() {
    insertFunctionString(BSFunFreq2FunButton);
}

void EventAttributesViewController::BSContinuumButtonClicked() {
    insertFunctionString(BSContinuumFunButton);
}

void EventAttributesViewController::insertFunctionString(FunctionButton button) {
    QLineEdit* target = nullptr;
    FunctionGenerator* gen = nullptr;

    switch (button) {
    case maxChildDurFunButton:
        target = ui->maxChildDurEntry;
        gen = new FunctionGenerator(nullptr, functionReturnFloat, target->text());
        break;
    case unitsPerSecondFunButton:
        target = ui->unitsPerSecondEntry;
        gen = new FunctionGenerator(nullptr, functionReturnInt, target->text());
        break;
    // case unitsPerBarFunButton:
    //     target = ui->unitsPerBarEntry;
    //     gen = new FunctionGenerator(functionReturnInt, target->text());
    //     break;
    case numOfChildFunButton1:
        target = ui->childCountEntry1;
        gen = new FunctionGenerator(nullptr, functionReturnInt, target->text());
        break;
    case numOfChildFunButton2:
        target = ui->childCountEntry2;
        gen = new FunctionGenerator(nullptr, functionReturnInt, target->text());
        break;
    case numOfChildFunButton3:
        target = ui->childCountEntry3;
        gen = new FunctionGenerator(nullptr, functionReturnInt, target->text());
        break;
    case childEventDefStartTimeFunButton:
        target = ui->childDefEntry1;
        gen = new FunctionGenerator(nullptr, functionReturnFloat, target->text());
        break;
    case childEventDefChildTypeFunButton:
        target = ui->childDefEntry2;
        gen = new FunctionGenerator(nullptr, functionReturnInt, target->text());
        break;
    case childEventDefDurationFunButton:
        target = ui->childDefEntry3;
        gen = new FunctionGenerator(nullptr, functionReturnFloat, target->text());
        break;
    case childEventDefAttackSieveFunButton:
        target = ui->attackSieveEntry;
        gen = new FunctionGenerator(nullptr, functionReturnSIV, target->text());
        break;
    case childEventDefDurationSieveFunButton:
        target = ui->durationSieveEntry;
        gen = new FunctionGenerator(nullptr, functionReturnSIV, target->text());
        break;
    case sieveBuilderFunButton:
        target = ui->sieveBuilderEntry;
        gen = new FunctionGenerator(nullptr, functionReturnSIV, target->text());
        break;
    case envBuilderFunButton:
        target = ui->envBuilderEntry;
        gen = new FunctionGenerator(nullptr, functionReturnENV, target->text());
        break;
    case attributesSpaFunButton:
        target = ui->spaEntry;
        gen = new FunctionGenerator(nullptr, functionReturnSPA, target->text());
        break;
    case attributesRevFunButton:
        target = ui->revEntry;
        gen = new FunctionGenerator(nullptr, functionReturnREV, target->text());
        break;
    case attributesFilFunButton:
        target = ui->filEntry;
        gen = new FunctionGenerator(nullptr, functionReturnFIL, target->text());
        break;
    case attributesFilBuilderFunButton:
        target = ui->filBuilderEntry;
        gen = new FunctionGenerator(nullptr, functionReturnFIL, target->text());
        break;
    case BSLoudnessFunButton:
        target = ui->loudnessEntry;
        gen = new FunctionGenerator(nullptr, functionReturnFloat, target->text());
        break;
    case BSModGroupFunButton:
        target = ui->modifierGroupEntry;
        gen = new FunctionGenerator(nullptr, functionReturnSPE, target->text());
        break;
    case BSWellTemperedFunButton:
        target = ui->wellTemperedEntry;
        gen = new FunctionGenerator(nullptr, functionReturnFloat, target->text());
        break;
    case BSFunFreq1FunButton:
        target = ui->funFreqEntry1;
        gen = new FunctionGenerator(nullptr, functionReturnFloat, target->text());
        break;
    case BSFunFreq2FunButton:
        target = ui->funFreqEntry2;
        gen = new FunctionGenerator(nullptr, functionReturnSPE, target->text());
        break;
    case BSContinuumFunButton:
        target = ui->continuumFreqEntry;
        gen = new FunctionGenerator(nullptr, functionReturnSPE, target->text());
        break;
    // case spectrumDeviationFunButton:
    //     target = ui->spectrumDeviationEntry;
    //     gen = new FunctionGenerator(functionReturnFloat, target->text());
    //     break;
    // case spectrumGenerateFunButton:
    //     target = ui->spectrumGenEntry;
    //     gen = new FunctionGenerator(functionReturnSPE, target->text());
    //     break;

    default:
        return;
    }

    if (gen) {
        if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty())
            target->setText(gen->getResultString());
        delete gen;
    }
}

void EventAttributesViewController::addLayerBoxUI(int layerIndex) {
    LayerBox* box = new LayerBox(m_curreventtype, m_curreventindex, layerIndex, this);
    connect(box, &LayerBox::deleteRequested, this, [this](LayerBox* b) {
        ProjectManager* pm2 = Inst::get_project_manager();
        HEvent* he = nullptr;
        if (m_curreventtype == top)        he = &pm2->topevent();
        else if (m_curreventtype == high)  he = &pm2->highevents()[m_curreventindex];
        else if (m_curreventtype == mid)   he = &pm2->midevents()[m_curreventindex];
        else if (m_curreventtype == low)   he = &pm2->lowevents()[m_curreventindex];
        else                               he = &pm2->bottomevents()[m_curreventindex].event;

        int idx = m_layerBoxes.indexOf(b);
        if (idx >= 0 && idx < he->event_layers.size()) {
            he->event_layers.removeAt(idx);
        }
        m_layerBoxes.removeOne(b);
        ui->layersLayout->removeWidget(b);
        b->deleteLater();
        for (int i = 0; i < m_layerBoxes.size(); ++i) {
            m_layerBoxes[i]->setLayerIndex(i);
        }
        fixStackedWidgetLayout(ui->standardPage);
    });
    box->setWeightEnabled(ui->byLayerButton->isChecked());
    box->setPackageFieldsVisible(ui->discreteButton->isChecked());
    m_layerBoxes.append(box);
    ui->layersLayout->addWidget(box);
}

void EventAttributesViewController::addNewLayerButtonClicked() {
    qDebug("add new layer button clicked");

    ProjectManager *pm = Inst::get_project_manager();

    HEvent* hevent = nullptr;
    if (m_curreventtype == top)         hevent = &pm->topevent();
    else if (m_curreventtype == high)   hevent = &pm->highevents()[m_curreventindex];
    else if (m_curreventtype == mid)    hevent = &pm->midevents()[m_curreventindex];
    else if (m_curreventtype == low)    hevent = &pm->lowevents()[m_curreventindex];
    else                                hevent = &pm->bottomevents()[m_curreventindex].event;

    hevent->event_layers.append(Layer());
    addLayerBoxUI(hevent->event_layers.size() - 1);
}

void EventAttributesViewController::addPartialsUI(int partialIndex) {
    Partials* par = new Partials(m_curreventindex, partialIndex, this);
    connect(par, &Partials::deleteRequested, this, [this](Partials* p) {
        qDebug() << "deleting in addPartialsUI";
        ProjectManager* pm2 = Inst::get_project_manager();
        QList<QString>& partials2 =  pm2->spectrumevents()[m_curreventindex].spectrum.partials;

        int idx = m_partials.indexOf(p);
        if (idx >= 0 && idx < partials2.size()) {
            partials2.removeAt(idx);
        }
        m_partials.removeAt(idx);
        ui->partialsLayout->removeWidget(p);
        p->deleteLater();
        for (int i = 0; i < m_partials.size(); ++i) {
            m_partials[i]->setPartialIndex(i);
        }
        fixStackedWidgetLayout(ui->soundPage);
        ui->spectrumNumPartialEntry->setText(QString::number(partials2.size()));
    });
    m_partials.append(par);
    ui->partialsLayout->addWidget(par);
    fixStackedWidgetLayout(ui->soundPage);

    ProjectManager *pm = Inst::get_project_manager();
    Spectrum* sevent = &pm->spectrumevents()[m_curreventindex].spectrum;
    ui->spectrumNumPartialEntry->setText(QString::number(sevent->partials.size()));
}


void EventAttributesViewController::addPartialButtonClicked() {
    qDebug("add new partial button clicked");

    ProjectManager *pm = Inst::get_project_manager();

    Spectrum* sevent = &pm->spectrumevents()[m_curreventindex].spectrum;
    sevent->partials.append("");

    addPartialsUI(sevent->partials.size()-1);
}

void EventAttributesViewController::addModifiersUI(int modifierIndex) {
    Modifiers* mod = new Modifiers(m_curreventtype, m_curreventindex, modifierIndex, this);
    connect(mod, &Modifiers::deleteRequested, this, [this](Modifiers* m) {
        ProjectManager* pm2 = Inst::get_project_manager();

        QList<Modifier>* modList = nullptr;
        if (m_curreventtype != bottom) {
            if (m_curreventtype == top)        modList = &pm2->topevent().modifiers;
            else if (m_curreventtype == high)  modList = &pm2->highevents()[m_curreventindex].modifiers;
            else if (m_curreventtype == mid)   modList = &pm2->midevents()[m_curreventindex].modifiers;
            else if (m_curreventtype == low)   modList = &pm2->lowevents()[m_curreventindex].modifiers;
        } else {
            modList = &pm2->bottomevents()[m_curreventindex].extra_info.modifiers;
        }

        int idx = m_modifiers.indexOf(m);
        if (modList && idx >= 0 && idx < modList->size()) {
            modList->removeAt(idx);
        }

        m_modifiers.removeOne(m);
        ui->modifiersLayout->removeWidget(m);
        m->deleteLater();
        for (int i = 0; i < m_modifiers.size(); ++i) {
            m_modifiers[i]->setModifierIndex(i);
        }
        fixStackedWidgetLayout(ui->standardPage);
    });

    m_modifiers.append(mod);
    ui->modifiersLayout->addWidget(mod);
    fixStackedWidgetLayout(ui->standardPage);
    
}

void EventAttributesViewController::addModifierButtonClicked() {
    qDebug("add new modifier button clicked");

    ProjectManager *pm = Inst::get_project_manager();

    if (m_curreventtype != bottom) {
        HEvent* hevent = nullptr;
        if (m_curreventtype == top)         hevent = &pm->topevent();
        else if (m_curreventtype == high)   hevent = &pm->highevents()[m_curreventindex];
        else if (m_curreventtype == mid)    hevent = &pm->midevents()[m_curreventindex];
        else if (m_curreventtype == low)    hevent = &pm->lowevents()[m_curreventindex];
        hevent->modifiers.append(Modifier());
        addModifiersUI(hevent->modifiers.size() - 1);
    } else {
        ExtraInfo* bevent = &pm->bottomevents()[m_curreventindex].extra_info;
        bevent->modifiers.append(Modifier());
        addModifiersUI(bevent->modifiers.size() - 1);
    }
}

void EventAttributesViewController::tempoAsNoteValueButtonClicked() {
    ui->tempoSecondaryStack->setCurrentWidget(ui->tempoValuePage);
    ui->emptyTempoPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->tempoFractionPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->tempoValuePage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->tempoValuePage->adjustSize();
    ui->tempoSecondaryStack->adjustSize();
    // bool modified = true;  // flag from original
    // if (!modified) {
    //     ui->tempoValueEntry->setText(ui->tempoValueEntry->text());
    // } else {
    //     QString s = generateTempoStringByFraction();
    //     FunctionGenerator gen(functionReturnFloat, s.toStdString());
    //     if (gen.run() == 0)
    //         ui->tempoValueEntry->setText(QString::fromStdString(gen.getResultString()));
    // }
}

void EventAttributesViewController::tempoAsFractionButtonClicked() {
    ui->tempoSecondaryStack->setCurrentWidget(ui->tempoFractionPage);
    ui->emptyTempoPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->tempoValuePage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->tempoFractionPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->tempoFractionPage->adjustSize();
    ui->tempoSecondaryStack->adjustSize();
    // leave text as is
}
/*
void EventAttributesViewController::tempoAsNoteValueEntryChanged(const QString&) {
    m_sharedPointers->projectView->modified();
}

void EventAttributesViewController::refreshChildTypeInLayer() {
    int idx = 0;
    for (auto* box : m_layerBoxesStorage)
        idx = box->refreshChildTypeIndex(idx);
}

bool EventAttributesViewController::deleteLayer(LayerBox* deleteBox) {
    if (m_layerBoxesStorage.size() <= 1) return false;
    ui->layersLayout->removeWidget(deleteBox);
    m_layerBoxesStorage.removeOne(deleteBox);
    delete deleteBox;
    return true;
}

IEvent* EventAttributesViewController::getCurrentEvent() const {
    return m_currentlyShownEvent;
}

bool EventAttributesViewController::checkAttackSieve() const {
    QString text = ui->attackSieveEntry->text();
    return m_sharedPointers->projectView
        ->getEventByTypeAndName(eventSiv, text.toStdString()) != nullptr;
}

bool EventAttributesViewController::checkDurationSieve() const {
    QString text = ui->durationSieveEntry->text();
    return m_sharedPointers->projectView
        ->getEventByTypeAndName(eventSiv, text.toStdString()) != nullptr;
}

void EventAttributesViewController::buildNoteModifiersList() {
    // clear old
    qDeleteAll(m_noteModifierCheckBoxes);
    m_noteModifierCheckBoxes.clear();
    // default
    auto defaults = m_sharedPointers->projectView->getDefaultNoteModifiers();
    int row = 0, col = 0;
    for (auto& kv : defaults) {
        if (!kv.second) continue;
        auto* cb = new QCheckBox(QString::fromStdString(kv.first));
        connect(cb, &QCheckBox::toggled, this, [=]{ m_sharedPointers->projectView->modified(); });
        ui->noteModifiersLayout->addWidget(cb, row, col++);
        if (col == 3) { col = 0; ++row; }
        m_noteModifierCheckBoxes.push_back(cb);
    }
    // custom
    auto customs = m_sharedPointers->projectView->getCustomNoteModifiers();
    for (auto& name : customs) {
        auto* cb = new QCheckBox(QString::fromStdString(name));
        connect(cb, &QCheckBox::toggled, this, [=]{ m_sharedPointers->projectView->modified(); });
        ui->noteModifiersLayout->addWidget(cb, row, col++);
        if (col == 3) { col = 0; ++row; }
        m_noteModifierCheckBoxes.push_back(cb);
    }
}

QString EventAttributesViewController::generateTempoStringByNoteValue() const {
    QString s;
    auto prefix = static_cast<TempoPrefix>(ui->tempoNotePrefixCombo->currentData().toInt());
    auto noteVal = static_cast<TempoNoteValue>(ui->tempoNoteValueCombo->currentData().toInt());
    switch (prefix) {
    case tempoPrefixDotted:         s += "dotted "; break;
    case tempoPrefixDoubleDotted:   s += "double dotted "; break;
    case tempoPrefixTriple:         s += "triple "; break;
    case tempoPrefixQuintuple:      s += "quintuple "; break;
    case tempoPrefixSextuple:       s += "sextuple "; break;
    case tempoPrefixSeptuple:       s += "septuple "; break;
    default: break;
    }
    switch (noteVal) {
    case tempoNoteValueWhole:        s += "whole = "; break;
    case tempoNoteValueHalf:         s += "half = "; break;
    case tempoNoteValueQuarter:      s += "quarter = "; break;
    case tempoNoteValueEighth:       s += "eighth = "; break;
    case tempoNoteValueSixteenth:    s += "sixteenth = "; break;
    case tempoNoteValueThirtySecond: s += "thirtysecond = "; break;
    }
    return s + ui->tempoValueEntry->text();
}

QString EventAttributesViewController::generateTempoStringByFraction() const {
    QString s;
    auto prefix = static_cast<TempoPrefix>(ui->tempoNotePrefixCombo->currentData().toInt());
    auto noteVal = static_cast<TempoNoteValue>(ui->tempoNoteValueCombo->currentData().toInt());
    switch (prefix) {
    case tempoPrefixDotted:         s += "dotted "; break;
    case tempoPrefixDoubleDotted:   s += "double dotted "; break;
    case tempoPrefixTriple:         s += "triple "; break;
    case tempoPrefixQuintuple:      s += "quintuple "; break;
    case tempoPrefixSextuple:       s += "sextuple "; break;
    case tempoPrefixSeptuple:       s += "septuple "; break;
    default: break;
    }
    switch (noteVal) {
    case tempoNoteValueWhole:        s += "whole = "; break;
    case tempoNoteValueHalf:         s += "half = "; break;
    case tempoNoteValueQuarter:      s += "quarter = "; break;
    case tempoNoteValueEighth:       s += "eighth = "; break;
    case tempoNoteValueSixteenth:    s += "sixteenth = "; break;
    case tempoNoteValueThirtySecond: s += "thirtysecond = "; break;
    }
    // convert fraction to denominator notation
    bool ok;
    int num = ui->tempoFractionEntry1->text().toInt(&ok);
    int den = ui->tempoValueEntry->text().toInt(&ok);
    return s + QString::number(num) + "/" + QString::number(den);
}

void EventAttributesViewController::deleteKeyPressed(QWidget* focus) {
    for (auto* box : m_layerBoxesStorage) {
        if (box->isAncestorOf(focus)) {
            box->deleteObject();
            break;
        }
    }
}*/
