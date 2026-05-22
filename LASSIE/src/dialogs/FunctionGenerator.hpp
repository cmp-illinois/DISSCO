#ifndef FUNCTIONGENERATOR_HPP
#define FUNCTIONGENERATOR_HPP

#include "../../../LASS/src/LASS.h"
#include "../lassie.hpp"

#include <QDialog>
#include <QMap>
#include <QWidget>
#include <QString>
#include <QLineEdit>

class FunctionWidget;
class SPAChannel;
class REVChannel;
class Stochos;
class Select;
class MakeEnvelopeRow;

namespace Ui {
class FunctionGenerator;
}

class FunctionGenerator : public QDialog
{
    Q_OBJECT

public:
    /*Constructor to create the Function Generator Pop Up*/
    explicit FunctionGenerator(QWidget *parent = nullptr, FunctionReturnType _returnType = FunctionReturnType::functionReturnFloat, QString _originalString = "");
    /*Destructor to delete the UI*/
    ~FunctionGenerator();
    
    Ui::FunctionGenerator *ui;

    
    // Store the type of combo box the generator should return
    FunctionReturnType returnType;
    // Store the inputted string
    QString originalString;
    // Store the result string
    QString result;
    // Returns the result string
    QString getResultString();
    // Changes the combo box to the right type
    void handleFunctionChanged(int index);

    // Signal handlers
    // Random
    void randomEntryChanged();
    void randomLowBoundFunButtonClicked();
    void randomHighBoundFunButtonClicked();

    // RandomInt
    void randomIntEntryChanged();
    void randomIntLowBoundFunButtonClicked();
    void randomIntHighBoundFunButtonClicked();

    // RandomOrderInt
    void randomOrderIntEntryChanged();
    void randomOrderIntLowBoundFunButtonClicked();
    void randomOrderIntHighBoundFunButtonClicked();

    // Randomizer
    void randomizerEntryChanged();
    void randomizerBaseFunButtonClicked();
    void randomizerDeviationFunButtonClicked();

    // RandomDensity
    void randomDensityEntryChanged();

    // Decay
    void decayBaseFunButtonClicked();
    void decayRateFunButtonClicked();
    void decayIndexFunButtonClicked();
    void decayEntryChanged();

    // Inverse
    void inverseFunButtonClicked();
    void inverseEntryChanged();

    // Markov
    void markovEntryChanged();

    // LN
    void LNFunButtonClicked();
    void LNEntryChanged();

    // Fibonacci
    void FibonacciFunButtonClicked();
    void FibonacciEntryChanged();

    // Select
    void selectIndexFunButtonClicked();
    void selectTextChanged();
    void addSelectNodeButtonClicked();
    void removeSelectNodeButtonClicked(Select* node);
    QLineEdit* m_lastFocusedSelectEdit = nullptr;
    void selectFunButtonClicked();

    // Stochos
    void stochosTextChanged();
    void addStochosNodeButtonClicked();
    void removeStochosNodeButtonClicked(Stochos* node);
    void clearStochosNodes();
    QLineEdit* m_lastFocusedStochosEdit = nullptr;
    void stochosFunButtonClicked();

    // ValuePick
    void valuePickAbsRangeFunButtonClicked();
    void valuePickLowFunButtonClicked();
    void valuePickHighFunButtonClicked();
    void valuePickDistFunButtonClicked();
    void valuePickTextChanged();

    // chooseL
    void chooseLFunButtonClicked();
    void chooseLEntryChanged();

    // GetPattern
    void getPatternOffsetFunButtonClicked();
    void getPatternFunButtonClicked();
    void getPatternEntryChanged();

    // MakeList
    void makeListFunctionFunButtonClicked();
    void makeListSizeFunButtonClicked();
    void makeListTextChanged();

    // EnvLib
    void envLibEnvelopeFunButtonClicked();
    void envLibScalingFactorFunButtonClicked();
    void envLibTextChanged();

    // MakeEnvelope
    QList<MakeEnvelopeRow*> m_envRows;
    MakeEnvelopeRow* envInsertRow(MakeEnvelopeRow* prevRow);
    void envRemoveRow(MakeEnvelopeRow* currRow);
    void clearEnvRows();
    void updateEnvRowLabels();
    void makeEnvelopeScalingFactorFunButtonClicked();
    void makeEnvelopeTextChanged();

    // MakePattern
    void makePatternIntervalsFunButtonClicked();
    void makePatternTextChanged();

    // ExpandPattern
    void expandPatternModuloFunButtonClicked();
    void expandPatternLowFunButtonClicked();
    void expandPatternHighFunButtonClicked();
    void expandPatternPatternFunButtonClicked();
    void expandPatternTextChanged();

    // MakeFilter
    void makeFilterFrequencyFunButtonClicked();
    void makeFilterBandWidthFunButtonClicked();
    void makeFilterDBGainFunButtonClicked();
    void makeFilterTextChanged();

    // REV
    int REVMethodFlag;
    QList<REVChannel*> m_revChannels;
    void REVTextChanged();
    void handleRevApplyMethodChanged();
    REVChannel* REVInsertChannel(REVChannel* prevCha);
    void REVRemoveChannel(REVChannel* currCha);
    void updateRevChaLabels();
    void clearRevChannels();

    
    // MakeSieve
    void makeSieveLowFunButtonClicked();
    void makeSieveHighFunButtonClicked();
    void makeSieveTextChanged();

    // SPA
    QList<SPAChannel*> m_spaChannels;
    void SPATextChanged();
    void handleSpaApplyMethodChanged();
    void handleSpaPartialSyncInsert(int index);
    void handleSpaPartialSyncRemove(int index);
    SPAChannel* SPAInsertChannel(SPAChannel* prevCha);
    void SPARemoveChannel(SPAChannel* currCha);
    void updateChaLabels();
    void clearSpaChannels();

    // Spectrum Gen
    void Spectrum_GenEnvelopeFunButtonClicked();
    void Spectrum_GenDistanceFunButtonClicked();
    void Spectrum_GenTextChanged();

    // Read Files
    void readENVFileTextChanged();
    void readSIVFileTextChanged();
    void readPATFileTextChanged();
    void readSPAFileTextChanged();
    void readREVFileTextChanged();
    void readFILFileTextChanged();

private:
    void setupUi();

    // Lazily build (and cache) the registered FunctionWidget for `id`,
    // appending it to functionStackedWidget on first use. Returns nullptr
    // if no subclass is registered for that id.
    FunctionWidget* ensureRegisteredWidget(CMODFunction id);

    // Set the current stacked-widget page to Expanding, the rest to
    // Ignored, then resize the dialog to fit the new page's sizeHint.
    void resizeToFitCurrentPage();

    QMap<CMODFunction, FunctionWidget*> m_registeredWidgets;
    QMap<CMODFunction, int> m_registeredPageIndex;
};

#endif // FUNCTIONGENERATOR_HPP