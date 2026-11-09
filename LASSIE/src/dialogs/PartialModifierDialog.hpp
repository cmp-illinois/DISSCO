#ifndef PARTIALMODIFIERDIALOG_HPP
#define PARTIALMODIFIERDIALOG_HPP

#include <QDialog>
#include <QString>
#include <QVector>

#include "PartialModifierFormat.hpp"
#include "../widgets/ModifierUiPolicy.hpp"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QGroupBox;
class QSpinBox;
class QScrollArea;
class QVBoxLayout;

/**
 * Structured editor for any Bottom modifier applied by PARTIAL.
 *
 * CMOD's legacy format stores four adjacent <Envelope> elements for every
 * partial, in probability/magnitude/width/rate order.  This dialog deliberately
 * keeps that wire format while showing only the parameters consumed by the
 * selected modifier type.  DETUNE stores plain numbers in the last three
 * slots: spread (magnitude), direction (width) and velocity (rate).
 */
class PartialModifierDialog : public QDialog
{
public:
    explicit PartialModifierDialog(QWidget* parent,
                                   int modifierType,
                                   const ModifierUiPolicy::PartialRowConstraint& constraint,
                                   const QString& originalString = QString());

    QString resultString() const;

protected:
    void accept() override;

private:
    struct PartialRow {
        QGroupBox* group = nullptr;
        QLineEdit* probability = nullptr;
        QLineEdit* magnitude = nullptr;
        QLineEdit* width = nullptr;
        QLineEdit* rate = nullptr;
    };

    void addPartialRow(int partialIndex,
                       const PartialModifierFormat::Values& values);
    void addEnvelopeEntry(QVBoxLayout* layout,
                          const QString& label,
                          const QString& value,
                          bool enabled,
                          bool numeric,
                          QLineEdit** entry);
    void setPartialRowCount(int count);
    void openEnvelopeGenerator(QLineEdit* entry);
    void updateCountControls();
    void updatePreview();

    int m_modifierType = 0;
    int m_activeRowCount = 0;
    int m_suggestedPartialCount = 1;
    int m_savedRowCount = 0;
    ModifierUiPolicy::PartialRowConstraint m_constraint;
    QVector<PartialRow> m_rows;
    QVBoxLayout* m_rowsLayout = nullptr;
    QSpinBox* m_rowCountSpin = nullptr;
    QPushButton* m_addPartialButton = nullptr;
    QPushButton* m_removePartialButton = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QLabel* m_countWarningLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QString m_statusContext;
    QPlainTextEdit* m_preview = nullptr;
};

#endif // PARTIALMODIFIERDIALOG_HPP
