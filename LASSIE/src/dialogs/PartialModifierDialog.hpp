#ifndef PARTIALMODIFIERDIALOG_HPP
#define PARTIALMODIFIERDIALOG_HPP

#include <QDialog>
#include "../dialogs/FunctionGenerator.hpp"
#include <QString>
#include <QVector>

class QVBoxLayout;
class QPushButton;
class QLineEdit;


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
        QLineEdit* probability = nullptr;
        QLineEdit* magnitude = nullptr;
        QLineEdit* width = nullptr;
        QLineEdit* rate = nullptr;
        QLineEdit* spread = nullptr;
        QLineEdit* direction = nullptr;
        QLineEdit* velocity = nullptr;
    };

    QVector<PartialRow> m_rows;

    QString buildPartialResultString() const;
    static QString envelopeOrNA(QLineEdit* edit);


    QVBoxLayout* m_mainLayout = nullptr;
    QVBoxLayout* m_rowsLayout = nullptr;
    QPushButton* m_addNodeButton = nullptr;
    QPushButton* m_insertFunctionButton = nullptr;
    QLineEdit* m_lastFocusedEdit = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;

    int m_nodeCount = 0;
    int m_modifierType = 0;
    int m_maxPartialCount = 0;

    QStringList enabledFieldLabels() const;
    void trackFocusedEdit(QLineEdit* edit);
    void populateFromResultString(const QString& resultString);
    static QStringList extractEnvelopeValues(const QString& resultString);

private slots:
    void addNode();
    void insertFunction();
};

#endif // PARTIALMODIFIERDIALOG_HPP