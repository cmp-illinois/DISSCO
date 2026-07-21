#ifndef PARTIALMODIFIERDIALOG_HPP
#define PARTIALMODIFIERDIALOG_HPP

#include <QDialog>
#include <QString>
#include <QVector>

#include "PartialModifierFormat.hpp"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

/**
 * Structured editor for a Bottom PHASE_MOD modifier applied by PARTIAL.
 *
 * CMOD's legacy format stores four adjacent <Envelope> elements for every
 * partial, in probability/magnitude/width/rate order.  This dialog deliberately
 * keeps that wire format while presenting the values as normal ENV function
 * entries instead of asking users to hand-author the XML wrapper.
 */
class PartialModifierDialog : public QDialog
{
public:
    explicit PartialModifierDialog(QWidget* parent,
                                   int spectrumPartialCount,
                                   const QString& originalString = QString());

    QString resultString() const;

private:
    struct PartialRow {
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
                          QLineEdit** entry);
    void openEnvelopeGenerator(QLineEdit* entry);
    void updatePreview();

    QVector<PartialRow> m_rows;
    QVBoxLayout* m_rowsLayout = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPlainTextEdit* m_preview = nullptr;
};

#endif // PARTIALMODIFIERDIALOG_HPP
