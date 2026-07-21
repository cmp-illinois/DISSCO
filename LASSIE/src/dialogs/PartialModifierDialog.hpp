#ifndef PARTIALMODIFIERDIALOG_HPP
#define PARTIALMODIFIERDIALOG_HPP

#include <QDialog>
#include "../dialogs/FunctionGenerator.hpp"
#include <QString>
#include <QVector>

class QVBoxLayout;
class QPushButton;
class QLineEdit;
class QLabel;
class QWidget;
class QPlainTextEdit;

class PartialModifierDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PartialModifierDialog(
    int modifierType,
    int maxPartialCount,
    const QString& existingResultString = QString(),
    QWidget* parent = nullptr
);
    QString getResultString() const;

private:
    struct PartialRow {
        QWidget* container = nullptr;
        QLabel* partialLabel = nullptr;

        QLabel* probabilityLabel = nullptr;
        QLabel* magnitudeLabel = nullptr;
        QLabel* widthLabel = nullptr;
        QLabel* rateLabel = nullptr;

        QLineEdit* probability = nullptr;
        QLineEdit* magnitude = nullptr;
        QLineEdit* width = nullptr;
        QLineEdit* rate = nullptr;
    };

    struct EnvelopeEnabled {
        bool probability = true;
        bool magnitude = true;
        bool width = true;
        bool rate = true;
    };    

    QVector<PartialRow> m_rows;

    EnvelopeEnabled enabledEnvelopeFields() const;
    void applyRowEnabledState(const PartialRow& row) const;

    void removeNode(QWidget* rowContainer);
    void renumberRows();

    QString buildPartialResultString() const;
    static QString envelopeOrNA(QLineEdit* edit);


    QVBoxLayout* m_mainLayout = nullptr;
    QVBoxLayout* m_rowsLayout = nullptr;
    QPushButton* m_addNodeButton = nullptr;
    QPushButton* m_insertFunctionButton = nullptr;
    QLineEdit* m_lastFocusedEdit = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;

    QPlainTextEdit* m_resultPreview = nullptr;

    int m_nodeCount = 0;
    int m_modifierType = 0;
    int m_maxPartialCount = 0;

    QStringList enabledFieldLabels() const;
    void trackFocusedEdit(QLineEdit* edit);
    void populateFromResultString(const QString& resultString);
    static QStringList extractEnvelopeValues(const QString& resultString);
    void updateResultPreview();

private slots:
    void addNode();
    void insertFunction();
};

#endif // PARTIALMODIFIERDIALOG_HPP