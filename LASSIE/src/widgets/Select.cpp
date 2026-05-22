#include "Select.hpp"
#include "../inst.hpp"

using enum FunctionReturnType;

Select::Select(int selectIndex, QWidget *parent)
    : QFrame(parent),
      m_selectIndex(selectIndex)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    setFrameShape(QFrame::NoFrame);

    m_row = new FunctionEntryRow("Value:", selectIndex, functionReturnMakeListFun, true, false, this);

    connect(m_row, &FunctionEntryRow::deleteRequested, this,
            [this](FunctionEntryRow*){ emit deleteRequested(this); });
    connect(m_row, &FunctionEntryRow::textChanged, this,
            [this](FunctionEntryRow*){ emit nodeTextChanged(this); });
    connect(m_row, &FunctionEntryRow::editFocused, this, &Select::editFocused);

    m_mainLayout->addWidget(m_row);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QString Select::getNodeText() {
    return m_row->getText();
}

Select::~Select() {}
