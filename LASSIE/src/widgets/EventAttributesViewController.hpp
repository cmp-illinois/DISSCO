#ifndef EVENTATTRIBUTESVIEWCONTROLLER_HPP
#define EVENTATTRIBUTESVIEWCONTROLLER_HPP

#include "../lassie.hpp"
#include <QFrame>
#include <QList>
#include <QString>
#include <QLabel>

#include "../core/event_struct.hpp"

class SharedPointers;
class ProjectView;
class EventBottomModifier;
class SpectrumPartial;
class EventLayer;
class QKeyEvent;
class Modifiers;
class LayerBox;
class Partials;

namespace Ui {
    class EventAttributesViewController;
}

typedef enum {
    maxChildDurFunButton,
    unitsPerSecondFunButton,
    unitsPerBarFunButton,
    numOfChildFunButton1,
    numOfChildFunButton2,
    numOfChildFunButton3,
    childEventDefStartTimeFunButton,
    childEventDefChildTypeFunButton,
    childEventDefDurationFunButton,
    childEventDefAttackSieveFunButton,
    childEventDefDurationSieveFunButton,
    spectrumDeviationFunButton,
    spectrumGenerateFunButton,
    attributesSpaFunButton,
    attributesRevFunButton,
    attributesFilFunButton,
    attributesFilBuilderFunButton,
    BSLoudnessFunButton,
    BSPhaseFunButton,
    BSModGroupFunButton,
    BSWellTemperedFunButton,
    BSFunFreq1FunButton,
    BSFunFreq2FunButton,
    BSContinuumFunButton,
    sieveBuilderFunButton,
    envBuilderFunButton
} FunctionButton;

// forward enums from original GTK version - now defined in LASSIE.h
// enum TempoPrefix;
// enum TempoNoteValue;
// enum ModifierType;

class EventAttributesViewController : public QFrame {
    Q_OBJECT

public:
    explicit EventAttributesViewController(ProjectView* projectView);
    // explicit EventAttributesViewController(SharedPointers* sharedPointers,
    //                                        QWidget* parent = nullptr);                                       
    ~EventAttributesViewController() override;
    ProjectView* e_projectView = nullptr;

    // /*! \brief shows the attributes of the event
    //  *  @param event The event to be shown
    //  */
    void showAttributesOfEvent(Eventtype type, int index); //IEvent* event

    void saveCurrentShownEventData();

    // Update the displayed name entry if typeStr/index matches the currently shown event
    void updateNameEntryIfShowing(const QString& typeStr, int index, const QString& name);

    // Rebuild all visible LayerBox widgets from the backend. Call after the
    // backend's discrete_packages have been mutated externally.
    void reloadAllLayerBoxes();

    // Blank the panel (e.g. when the viewed event is deleted)
    void clearView();

    // Called by ProjectView after an event is removed from the backend; adjusts
    // the tracked index or clears the view if the deleted event was being shown.
    void onEventDeleted(Eventtype type, int deletedIndex);

    // /*! \brief get the currently shown event */
    // IEvent* getCurrentEvent() const;

    // /*! \brief checks whether attack sieve name refers to existing sieve */
    // bool checkAttackSieve() const;

    // /*! \brief checks whether duration sieve name refers to existing sieve */
    // bool checkDurationSieve() const;

protected:
    // void keyPressEvent(QKeyEvent* event) override;

private slots:
    // // mode buttons
    void fixedButtonClicked();
    void densityButtonClicked();
    void byLayerButtonClicked();

    // // child-event-def buttons
    void continuumButtonClicked();
    void sweepButtonClicked();
    void discreteButtonClicked();

    // // insert-function buttons
    void maxChildDurFunButtonClicked();
    void unitsPerSecondFunButtonClicked();
    void numOfChildFunButton1Clicked();
    void numOfChildFunButton2Clicked();
    void numOfChildFunButton3Clicked();
    void childEventDefStartTimeFunButtonClicked();
    void childEventDefChildTypeFunButtonClicked();
    void childEventDefDurationFunButtonClicked();
    void childEventAttackSieveButtonClicked();
    void childEventDurationSieveButtonClicked();
    void sieveBuilderButtonClicked();
    void envBuilderFunButtonClicked();
    // void deviationFunButtonClicked();
    // void generatespectrumFunButtonClicked();

    // // bottom-sub attribute function buttons
    void attributesStandardRevButtonClicked();
    void attributesStandardFilButtonClicked();
    void attributesFilBuilderButtonClicked();
    void attributesStandardSpaButtonClicked();
    void BSLoudnessButtonClicked();
    void BSPhaseButtonClicked();
    // void BSSpatializationButtonClicked();
    // void BSReverbButtonClicked();
    // void BSFilterButtonClicked();
    void BSModifierGroupButtonClicked();

    void wellTemperedRadioButtonClicked();
    void fundamentalRadioButtonClicked();
    void continuumRadioButtonClicked();
    void BSWellTemperedButtonClicked();
    void BSFunFreqButton1Clicked();
    void BSFunFreqButton2Clicked();
    void BSContinuumButtonClicked();

    // // main actions
    void addNewLayerButtonClicked();
    void addModifierButtonClicked();
    void addPartialButtonClicked();

    // // tempo controls
    void tempoAsNoteValueButtonClicked();
    void tempoAsFractionButtonClicked();
    // void tempoAsNoteValueEntryChanged(const QString&);

private:
    // UI loader
    Ui::EventAttributesViewController* ui;

    // // data/model
    // SharedPointers*              m_sharedPointers;
    // IEvent*                      m_currentlyShownEvent;
    // Eventtype                       m_currentlyShownEvent;
    
    Eventtype m_curreventtype;
    // index of event in QList in ProjectManager
    int m_curreventindex = -1;
    bool m_hasCurrentEvent = false;
    // class LayerBox*              m_modifiers;             // head of doubly-linked modifiers
    QList<Modifiers*>               m_modifiers;  
    QList<Partials*>                m_partials;   
    // class SoundPartialHBox*      m_soundPartialHboxes;    // head of doubly-linked partials

    QList<LayerBox*>                 m_layerBoxes;
    // QList<class QCheckBox*>          m_noteModifierCheckBoxes;

    // // internal helpers
    void showCurrentEventData();
    // Creates a LayerBox UI for the given layerIndex and attaches it to the panel.
    // Does NOT touch the backend — callers are responsible for ensuring the backend
    // layer already exists at that index before calling this.
    void addLayerBoxUI(int layerIndex);
    void addPartialsUI(int partialIndex);
    void addModifiersUI(int modifierIndex);
    void insertFunctionString(FunctionButton fn);
    void fixStackedWidgetLayout(QWidget* currPage);
};



// // BottomEventModifierAlignment: UI + logic for a single modifier
// class BottomEventModifierAlignment : public QWidget {
//     Q_OBJECT

// public:
//     BottomEventModifierAlignment(EventBottomModifier* modifier,
//                                  EventAttributesViewController* parentView);
//     ~BottomEventModifierAlignment() override;

//     /*! \brief write all fields back into the EventBottomModifier and its successors */
//     void saveToEvent();

//     BottomEventModifierAlignment* prev { nullptr };
//     BottomEventModifierAlignment* next { nullptr };

// private slots:
//     void onApplyHowChanged(int index);
//     void onTypeChanged(int index);
//     void onRemoveClicked();
//     void probablityEnvelopeButtonClicked();
//     void ampValueEnvelopeButtonClicked();
//     void rateValueEnvelopeButtonClicked();
//     void widthEnvelopeButtonClicked();
//     void partialButtonClicked();
//     void modified();

// private:
//     void grayOutModifierFields(ModifierType type);

//     EventAttributesViewController* m_attributesView;
//     EventBottomModifier*           m_modifier;
//     // class Ui::ModifierWidget*      ui;  // holds pointers to comboboxes, entries, etc.
// };

// // SoundPartialHBox: UI + logic for a single spectrum partial
// class SoundPartialHBox : public QWidget {
//     Q_OBJECT

// public:
//     SoundPartialHBox(SpectrumPartial* partial,
//                      EventAttributesViewController* parentView);
//     ~SoundPartialHBox() override;

//     /*! \brief remove all successors and delete this */
//     void clear();

//     /*! \brief write back envString into SpectrumPartial chain */
//     void saveString();

//     /*! \brief set the displayed "Partial N" label, recursing to next */
//     void setPartialNumber(int number);

//     SoundPartialHBox* prev;
//     SoundPartialHBox* next;

// private slots:
//     void removeButtonClicked();
//     void functionButtonClicked();
//     void modified();

// private:
//     SpectrumPartial*                     m_partial;
//     EventAttributesViewController*       m_attributesView;

//     // UI elements
//     QHBoxLayout*                         m_layout;
//     QLabel*                              m_label;
//     QLineEdit*                           m_envelopeEntry;
//     QPushButton*                         m_functionButton;
//     QPushButton*                         m_removeButton;
// };

#endif // EVENTATTRIBUTESVIEWCONTROLLER_HPP
