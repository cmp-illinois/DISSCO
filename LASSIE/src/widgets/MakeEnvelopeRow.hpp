#ifndef MAKE_ENVELOPE_ROW_HPP
#define MAKE_ENVELOPE_ROW_HPP

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QString>

#include "../lassie.hpp"

class MakeEnvelopeRow : public QFrame {
    Q_OBJECT

public:
    MakeEnvelopeRow(int index, QWidget* parent = nullptr);
    ~MakeEnvelopeRow() override;

    int     getIndex() const { return m_index; }
    void    setIndex(int index);

    QString getX() const;
    QString getY() const;
    QString getType() const;
    QString getPro() const;

    void setX(const QString& text);
    void setY(const QString& text);
    void setType(const QString& type);
    void setPro(const QString& pro);

    void setLabel(const QString& text);

signals:
    void deleteRequested(MakeEnvelopeRow* self);
    void insertRequested(MakeEnvelopeRow* self);
    void textChanged(MakeEnvelopeRow* self);

private slots:
    void onXFnClicked();
    void onYFnClicked();
    void onRmClicked();
    void onInsClicked();
    void onChanged();

private:
    int m_index;

    QHBoxLayout* m_hBox      = nullptr;
    QLabel*      m_label     = nullptr;
    QLineEdit*   m_xEdit     = nullptr;
    QPushButton* m_xFnBtn    = nullptr;
    QLineEdit*   m_yEdit     = nullptr;
    QPushButton* m_yFnBtn    = nullptr;
    QComboBox*   m_typeCombo = nullptr;
    QComboBox*   m_proCombo  = nullptr;
    QPushButton* m_rmBtn     = nullptr;
    QPushButton* m_insBtn    = nullptr;
};

#endif
