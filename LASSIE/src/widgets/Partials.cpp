#include "Partials.hpp"
#include "../inst.hpp"

using enum FunctionReturnType;

Partials::Partials(unsigned eventIndex, int partialIndex, QWidget* parent)
    : QFrame(parent),
      m_eventIndex(eventIndex),
      m_partialIndex(partialIndex)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    setFrameShape(QFrame::NoFrame);

    m_row = new FunctionEntryRow(
        "Partial " + QString::number(partialIndex+1) + ":",
        partialIndex,
        functionReturnENV,
        true,
        false,
        this);

    // Populate UI from backend before connecting signals (avoids spurious backend write on init)
    m_row->setText(getBackendLayer().partials[m_partialIndex]);

    connect(m_row, &FunctionEntryRow::deleteRequested, this,
            [this](FunctionEntryRow*){ emit deleteRequested(this); });
    connect(m_row, &FunctionEntryRow::textChanged, this,
            [this](FunctionEntryRow*){ getBackendLayer().partials[m_partialIndex] = m_row->getText(); });

    m_mainLayout->addWidget(m_row);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}


Spectrum& Partials::getBackendLayer() {
    ProjectManager* pm = Inst::get_project_manager();
    return (&pm->spectrumevents()[m_eventIndex])->spectrum;
}


Partials::~Partials() {}
