#ifndef MODIFIERS_HPP
#define MODIFIERS_HPP

#include <QWidget>
#include <QString>
#include <QFrame>

#include "../core/event_struct.hpp"

typedef enum {
    modProbabilityChanged,
    modMagnitudeChanged,
    modRateChanged,
    modWidthChanged,
    modPartialChanged,
    modSpreadChanged,
    modDirChanged,
    modVelChanged,
    modNameChanged
} ModChanged;


class EventAttributesViewController;

namespace Ui {
class Modifiers;
}

class Modifiers : public QFrame
{
    Q_OBJECT

public:
    /*Constructor to create the modifier*/
    Modifiers(Eventtype eventType, unsigned eventIndex, int modifierIndex, QWidget *parent = nullptr);
    /*Destructor to delete the UI*/
    ~Modifiers() override;
    void setModifierIndex(int modifierIndex) {  m_modifierIndex = modifierIndex; }
    void saveModifierToBackend();
    void setModifierData(Modifier& modData);
    
    Ui::Modifiers *ui;

signals:
    void deleteRequested(Modifiers* self);

private slots:
    void modFunctionButtonClicked(ModChanged type);
    void modRemoveButtonClicked();
    void modTextChanged(ModChanged type);

private:
    void setupUi();
    void updateModState();


    void updateFieldsForApplyMode();
    void setSoundFieldsFromBackend();
    void clearSoundFieldsForPartialMode();
    void updateApplyOptionsForModifierType();

    int maxPartialCountForCurrentBottom() const;
    
    Modifier& getBackendLayer();

    Eventtype m_eventType;
    unsigned  m_eventIndex;
    int       m_modifierIndex;


};

#endif // MODIFIERS_HPP