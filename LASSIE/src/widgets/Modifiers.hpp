#ifndef MODIFIERS_HPP
#define MODIFIERS_HPP

#include <QFrame>

class QPushButton;

#include "../core/event_struct.hpp"

namespace Ui {
class Modifiers;
}

/**
 * Compact ordered row for one configured Modifier instance.
 *
 * Activation settings stay visible in the list. Detailed synthesis fields and
 * conditional exceptions are edited atomically in dialogs.
 */
class Modifiers : public QFrame
{
    Q_OBJECT

public:
    Modifiers(Eventtype eventType, unsigned eventIndex, int modifierIndex,
              QWidget* parent = nullptr);
    ~Modifiers() override;

    void setModifierIndex(int modifierIndex);
    void saveModifierToBackend();

signals:
    void deleteRequested(Modifiers* self);
    void moveUpRequested(Modifiers* self);
    void moveDownRequested(Modifiers* self);
    void dataChanged();

private:
    Modifier& backendModifier();
    QList<Modifier>& backendModifierList();
    int currentModifierType() const;
    void updateRow();
    void openParameters();
    void openRules();
    void openDefaultOnEnvelope();
    void useDefaultOnNumber();

    Ui::Modifiers* ui;
    // Default ON can be a fixed chance (spin box) or an envelope.
    QPushButton* m_defaultEnvelopeButton = nullptr;
    QPushButton* m_defaultNumberButton = nullptr;
    Eventtype m_eventType;
    unsigned m_eventIndex;
    int m_modifierIndex;
};

#endif // MODIFIERS_HPP
