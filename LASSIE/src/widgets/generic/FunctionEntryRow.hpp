#ifndef FUNCTION_ENTRY_ROW_HPP
#define FUNCTION_ENTRY_ROW_HPP

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>

#include "../../lassie.hpp"

// A generic row widget: [Label | QLineEdit | fn button | rm button | ins button]
// The fn button opens a FunctionGenerator dialog of the configured return type.
// Consumers connect to deleteRequested, textChanged, and/or editFocused.
class FunctionEntryRow : public QFrame {
    Q_OBJECT

public:
    FunctionEntryRow(const QString& labelText,
                     int index,
                     FunctionReturnType fnReturnType,
                     bool rmVisible,
                     bool insVisible,
                     QWidget* parent = nullptr,
                     bool fnVisible = true);
    ~FunctionEntryRow() override;

    int     getIndex() const { return m_index; }
    void    setIndex(int index) { m_index = index; }

    QString getText() const;
    void    setText(const QString& text);

    // Update the label text (e.g. for numbered rows like "Partial 1:")
    void    setLabel(const QString& text);

signals:
    void deleteRequested(FunctionEntryRow* self);
    void textChanged(FunctionEntryRow* self);
    void insertRequested(FunctionEntryRow* self);
    // Emitted whenever the cursor moves into the entry — useful if a parent
    // wants to track which entry is "active" for an external fn button.
    void editFocused(QLineEdit* edit);

private slots:
    void onFnClicked();
    void onRmClicked();
    void onInsClicked();
    void onTextChanged();

private:
    int                 m_index;
    FunctionReturnType  m_fnReturnType;

    QHBoxLayout*    m_hBox      = nullptr;
    QLabel*         m_label     = nullptr;
    QLineEdit*      m_entry     = nullptr;
    QPushButton*    m_fnButton  = nullptr;
    QPushButton*    m_rmButton  = nullptr;
    QPushButton*    m_insButton  = nullptr;
};

#endif
