#include "FunctionGenerator.hpp"
#include "../ui/ui_FunctionGenerator.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QDialogButtonBox>
// #include <QOverload>
#include <QDateTime>
#include <QScrollBar>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <string>

#include "../widgets/SPAChannel.hpp"
#include "../widgets/REVChannel.hpp"

#include "../widgets/Stochos.hpp"
#include "../widgets/Select.hpp"

#include "functions/FunctionRegistry.hpp"
#include "functions/FunctionWidget.hpp"

using enum FunctionReturnType;
using enum CMODFunction;

namespace {
    // Capture inner content of element at StartElement; leave cursor at EndElement.
    // For text-only elements returns the text; for elements with nested children
    // returns the serialized inner XML (e.g. "<Fun>...</Fun>").
    QString readInner(QXmlStreamReader& r) {
        QString result;
        QXmlStreamWriter w(&result);
        bool hasChildElement = false;
        QString textBuffer;
        int depth = 0;
        while (!r.atEnd()) {
            r.readNext();
            switch (r.tokenType()) {
                case QXmlStreamReader::StartElement: {
                    hasChildElement = true;
                    w.writeStartElement(r.name().toString());
                    const auto attrs = r.attributes();
                    for (const auto& a : attrs)
                        w.writeAttribute(a.name().toString(), a.value().toString());
                    ++depth;
                    break;
                }
                case QXmlStreamReader::EndElement:
                    if (depth == 0)
                        return hasChildElement ? result : textBuffer;
                    w.writeEndElement();
                    --depth;
                    break;
                case QXmlStreamReader::Characters:
                    if (hasChildElement) w.writeCharacters(r.text().toString());
                    else textBuffer += r.text().toString();
                    break;
                default: break;
            }
        }
        return hasChildElement ? result : textBuffer;
    }

    QString nextChildInner(QXmlStreamReader& r) {
        if (!r.readNextStartElement()) return QString();
        return readInner(r);
    }
}


FunctionGenerator::FunctionGenerator(QWidget *parent, FunctionReturnType _returnType, QString _originalString)
    : QDialog(parent)
    , ui(new Ui::FunctionGenerator)
{
    // Sets up the user interface specified by the .ui file
    result = "";
    returnType = _returnType;
    originalString = _originalString;
    ui->setupUi(this);
    for (int i = 0; i < ui->functionStackedWidget->count(); ++i) {
      ui->functionStackedWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
    ui->functionStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->layout()->setSizeConstraint(QLayout::SetDefaultConstraint);
    setupUi();
    this->adjustSize();
    qDebug() << "returnType in FuncGen:" << returnType;
}

FunctionGenerator::~FunctionGenerator()
{
    delete ui;
}

// Lazily build a registered FunctionWidget on first reference and cache
// it. Up-front instantiation made every dialog open pay the cost of
// every registered subclass (13+ widgets, mostly unused per session) and
// perturbed the stacked widget's layout, both visibly.
FunctionWidget* FunctionGenerator::ensureRegisteredWidget(CMODFunction id)
{
    if (auto it = m_registeredWidgets.find(id); it != m_registeredWidgets.end())
        return *it;
    FunctionWidget* w = FunctionRegistry::instance().create(id);
    if (!w) return nullptr;
    // Sizing policy is managed per-selection by resizeToFitCurrentPage(),
    // which sets the current page to Expanding and the rest to Ignored.
    const int pageIndex = ui->functionStackedWidget->addWidget(w);
    m_registeredWidgets.insert(id, w);
    m_registeredPageIndex.insert(id, pageIndex);
    connect(w, &FunctionWidget::xmlChanged, this, [this, w]() {
        ui->resultTextEdit->setText(w->buildXMLString());
    });
    return w;
}

// Mirrors the inline resize dance at the tail of handleFunctionChanged:
// expand the active page, ignore the rest, then shrink the dialog to fit.
void FunctionGenerator::resizeToFitCurrentPage()
{
    const int current = ui->functionStackedWidget->currentIndex();
    setMinimumSize(0, 0);
    for (int i = 0; i < ui->functionStackedWidget->count(); ++i) {
        ui->functionStackedWidget->widget(i)->setSizePolicy(
            i == current ? QSizePolicy::Expanding : QSizePolicy::Ignored,
            i == current ? QSizePolicy::Expanding : QSizePolicy::Ignored);
    }
    ui->functionStackedWidget->layout()->activate();
    layout()->activate();
    const QSize ideal = sizeHint();
    resize(ideal);
    setMinimumSize(ideal);
}

void FunctionGenerator::setupUi()
{
    ui->functionOptions->clear();
    // Add combo box function options
    if (returnType == functionReturnInt){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("RandomInt", static_cast<int>(functionRandomInt)); // index 2
        ui->functionOptions->addItem("RandomOrderInt", static_cast<int>(functionRandomOrderInt)); // index 3
        ui->functionOptions->addItem("Stochos", static_cast<int>(functionStochos)); // index 11
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
        ui->functionOptions->addItem("ValuePick", static_cast<int>(functionValuePick)); // index 13
        ui->functionOptions->addItem("ChooseL", static_cast<int>(functionChooseL)); // index 14
        ui->functionOptions->addItem("GetPattern", static_cast<int>(functionGetPattern)); // index 15
        ui->functionOptions->addItem("GetFromMarkovChain", static_cast<int>(functionMarkov)); // index 10
        ui->functionOptions->addItem("Fibonacci", static_cast<int>(functionFibonacci)); // index 8
        ui->functionOptions->addItem("-------", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("CURRENT_TYPE", static_cast<int>(function_staticCURRENT_TYPE)); // index 0
        ui->functionOptions->addItem("CURRENT_CHILD_NUM", static_cast<int>(function_staticCURRENT_CHILD_NUM)); // index 0
        ui->functionOptions->addItem("CURRENT_PARTIAL_NUM", static_cast<int>(function_staticCURRENT_PARTIAL_NUM)); // index 0
        ui->functionOptions->addItem("CURRENT_SEGMENT", static_cast<int>(function_staticCURRENT_SEGMENT)); // index 0
        ui->functionOptions->addItem("AVAILABLE_EDU", static_cast<int>(function_staticAVAILABLE_EDU)); // index 0
        ui->functionOptions->addItem("PREVIOUS_CHILD_DURATION", static_cast<int>(function_staticPREVIOUS_CHILD_DURATION)); // index 0
        ui->functionOptions->addItem("CURRENT_LAYER", static_cast<int>(function_staticCURRENT_LAYER)); // index 0
    }
    else if (returnType == functionReturnFloat){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("Random", static_cast<int>(functionRandom)); // index 1
        ui->functionOptions->addItem("RandomInt", static_cast<int>(functionRandomInt)); // index 2
        ui->functionOptions->addItem("RandomOrderInt", static_cast<int>(functionRandomOrderInt)); // index 3
        ui->functionOptions->addItem("Randomizer", static_cast<int>(functionRandomizer)); // index 4
        ui->functionOptions->addItem("RandomDensity", static_cast<int>(functionRandomDensity)); // index 5
        ui->functionOptions->addItem("Inverse", static_cast<int>(functionInverse)); // index 6
        ui->functionOptions->addItem("LN", static_cast<int>(functionLN)); // index 7
        ui->functionOptions->addItem("Fibonacci", static_cast<int>(functionFibonacci)); // index 8
        ui->functionOptions->addItem("Decay", static_cast<int>(functionDecay)); // index 9
        ui->functionOptions->addItem("GetFromMarkovChain", static_cast<int>(functionMarkov)); // index 10
        ui->functionOptions->addItem("-------", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("Stochos", static_cast<int>(functionStochos)); // index 11
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
        ui->functionOptions->addItem("ValuePick", static_cast<int>(functionValuePick)); // index 13
        ui->functionOptions->addItem("ChooseL", static_cast<int>(functionChooseL)); // index 14
        ui->functionOptions->addItem("GetPattern", static_cast<int>(functionGetPattern)); // index 15
        ui->functionOptions->addItem("-------", static_cast<int>(NOT_A_FUNCTION));  // index 0
        ui->functionOptions->addItem("CURRENT_TYPE", static_cast<int>(function_staticCURRENT_TYPE)); // index 0
        ui->functionOptions->addItem("CURRENT_CHILD_NUM", static_cast<int>(function_staticCURRENT_CHILD_NUM)); // index 0
        ui->functionOptions->addItem("CURRENT_PARTIAL_NUM", static_cast<int>(function_staticCURRENT_PARTIAL_NUM)); // index 0
        ui->functionOptions->addItem("CURRENT_SEGMENT", static_cast<int>(function_staticCURRENT_SEGMENT)); // index 0
        ui->functionOptions->addItem("AVAILABLE_EDU", static_cast<int>(function_staticAVAILABLE_EDU)); // index 0
        ui->functionOptions->addItem("PREVIOUS_CHILD_DURATION", static_cast<int>(function_staticPREVIOUS_CHILD_DURATION)); // index 0
        ui->functionOptions->addItem("CURRENT_LAYER", static_cast<int>(function_staticCURRENT_LAYER)); // index 0
    } else if (returnType == functionReturnMakeListFun) {
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("Random", static_cast<int>(functionRandom)); // index 1
        ui->functionOptions->addItem("RandomInt", static_cast<int>(functionRandomInt)); // index 2
        ui->functionOptions->addItem("RandomOrderInt", static_cast<int>(functionRandomOrderInt)); // index 3
        ui->functionOptions->addItem("Randomizer", static_cast<int>(functionRandomizer)); // index 4
        ui->functionOptions->addItem("RandomDensity", static_cast<int>(functionRandomDensity)); // index 5
        ui->functionOptions->addItem("Inverse", static_cast<int>(functionInverse)); // index 6
        ui->functionOptions->addItem("LN", static_cast<int>(functionLN)); // index 7
        ui->functionOptions->addItem("Fibonacci", static_cast<int>(functionFibonacci)); // index 8
        ui->functionOptions->addItem("Decay", static_cast<int>(functionDecay)); // index 9
        ui->functionOptions->addItem("-------", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("Stochos", static_cast<int>(functionStochos)); // index 11
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
        ui->functionOptions->addItem("ValuePick", static_cast<int>(functionValuePick)); // index 13
        ui->functionOptions->addItem("ChooseL", static_cast<int>(functionChooseL)); // index 14
        ui->functionOptions->addItem("GetPattern", static_cast<int>(functionGetPattern)); // index 15
        ui->functionOptions->addItem("EnvLib", static_cast<int>(functionEnvLib)); // index 16
        ui->functionOptions->addItem("MakeEnvelope", static_cast<int>(functionMakeEnvelope)); // index 17
        ui->functionOptions->addItem("ReadENVFile", static_cast<int>(functionReadENVFile)); // index 18
        ui->functionOptions->addItem("-------", static_cast<int>(NOT_A_FUNCTION));  // index 0
        ui->functionOptions->addItem("CURRENT_TYPE", static_cast<int>(function_staticCURRENT_TYPE)); // index 0
        ui->functionOptions->addItem("CURRENT_CHILD_NUM", static_cast<int>(function_staticCURRENT_CHILD_NUM)); // index 0
        ui->functionOptions->addItem("CURRENT_PARTIAL_NUM", static_cast<int>(function_staticCURRENT_PARTIAL_NUM)); // index 0
        ui->functionOptions->addItem("CURRENT_SEGMENT", static_cast<int>(function_staticCURRENT_SEGMENT)); // index 0
        ui->functionOptions->addItem("AVAILABLE_EDU", static_cast<int>(function_staticAVAILABLE_EDU)); // index 0
        ui->functionOptions->addItem("PREVIOUS_CHILD_DURATION", static_cast<int>(function_staticPREVIOUS_CHILD_DURATION)); // index 0
        ui->functionOptions->addItem("CURRENT_LAYER", static_cast<int>(function_staticCURRENT_LAYER)); // index 0
    } else if (returnType == functionReturnList) {
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION));
        ui->functionOptions->addItem("MakeList", static_cast<int>(functionMakeList)); // index 19
        ui->functionOptions->addItem("RawList", static_cast<int>(functionRawList)); // index 0
    } else if (returnType == functionReturnENV) {
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("MakeEnvelope", static_cast<int>(functionMakeEnvelope)); // index 17
        ui->functionOptions->addItem("ReadENVFile", static_cast<int>(functionReadENVFile)); // index 18
        ui->functionOptions->addItem("EnvLib", static_cast<int>(functionEnvLib)); // index 16
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
        ui->functionOptions->addItem("Stochos", static_cast<int>(functionStochos)); // index 11
    } else if (returnType == functionReturnSPA){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("SPA", static_cast<int>(functionSPA)); // index 20
        ui->functionOptions->addItem("ReadSPAFile", static_cast<int>(functionReadSPAFile)); // index 21
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
    } else if (returnType == functionReturnFIL){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("MakeFilter", static_cast<int>(functionMakeFilter)); // index 22
        ui->functionOptions->addItem("ReadFILFile", static_cast<int>(functionReadFILFile)); // index 23
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
    } else if (returnType == functionReturnMGP){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
    } else if (returnType == functionReturnPAT){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("MakePattern", static_cast<int>(functionMakePattern)); // index 24
        ui->functionOptions->addItem("ExpandPattern", static_cast<int>(functionExpandPattern)); // index 25
        ui->functionOptions->addItem("ReadPATFile", static_cast<int>(functionReadPATFile)); // index 26
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
    } else if (returnType == functionReturnREV){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("REV_Simple", static_cast<int>(functionREV_Simple)); // index 27
        ui->functionOptions->addItem("REV_Medium", static_cast<int>(functionREV_Medium)); // index 27
        ui->functionOptions->addItem("REV_Advanced", static_cast<int>(functionREV_Advanced)); // index 27
        ui->functionOptions->addItem("ReadREVFile", static_cast<int>(functionReadREVFile)); // index 28
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
    } else if (returnType == functionReturnSPE){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("Generate_Spectrum", static_cast<int>(functionSpectrum_Gen)); // index 29
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
    } else if (returnType == functionReturnSIV){
        ui->functionOptions->addItem("", static_cast<int>(NOT_A_FUNCTION)); // index 0
        ui->functionOptions->addItem("MakeSieve", static_cast<int>(functionMakeSieve)); // index 30
        ui->functionOptions->addItem("ReadSIVFile", static_cast<int>(functionReadSIVFile)); // index 31
        ui->functionOptions->addItem("Select", static_cast<int>(functionSelect)); // index 12
    }

    // Handles combo box option selected by calling handleFunctionChanged
    connect(ui->functionOptions, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &FunctionGenerator::handleFunctionChanged);
    ui->functionOptions->setCurrentIndex(0);
    this->adjustSize();

    // Centers result label
    ui->resultLabel->setAlignment(Qt::AlignCenter);
    // Sets height of result box
    ui->resultTextEdit->setMinimumHeight(60);
    ui->resultTextEdit->setMaximumHeight(100);
    // Connects the ui OK and CANCEL buttons to their corresponding actions
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Random signals
    connect(ui->randomLowerBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomLowBoundFunButtonClicked);
    connect(ui->randomUpperBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomHighBoundFunButtonClicked);
    connect(ui->randomLowerBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomEntryChanged);
    connect(ui->randomUpperBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomEntryChanged);

    // RandomInt signals
    connect(ui->randomIntLowerBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomIntLowBoundFunButtonClicked);
    connect(ui->randomIntUpperBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomIntHighBoundFunButtonClicked);
    connect(ui->randomIntLowerBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomIntEntryChanged);
    connect(ui->randomIntUpperBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomIntEntryChanged);

    // RandomOrderInt signals
    connect(ui->randomOrderIntLowerBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomOrderIntLowBoundFunButtonClicked);
    connect(ui->randomOrderIntUpperBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomOrderIntHighBoundFunButtonClicked);
    connect(ui->randomOrderIntLowerBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomOrderIntEntryChanged);
    connect(ui->randomOrderIntUpperBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomOrderIntEntryChanged);

    // Randomizer signals
    connect(ui->randomizerBaseInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomizerBaseFunButtonClicked);
    connect(ui->randomizerDeviationInsertFn, &QPushButton::clicked, this, &FunctionGenerator::randomizerDeviationFunButtonClicked);
    connect(ui->randomizerBaseEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomizerEntryChanged);
    connect(ui->randomizerDeviationEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomizerEntryChanged);

    // RandomDensity signals
    connect(ui->randomDensityEnvEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomDensityEntryChanged);
    connect(ui->randomDensityStartTimeEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomDensityEntryChanged);
    connect(ui->randomDensityEndTimeEdit, &QLineEdit::textChanged, this, &FunctionGenerator::randomDensityEntryChanged);

    // Decay signals
    connect(ui->decayBaseInsertFn, &QPushButton::clicked, this, &FunctionGenerator::decayBaseFunButtonClicked);
    connect(ui->decayRateInsertFn, &QPushButton::clicked, this, &FunctionGenerator::decayRateFunButtonClicked);
    connect(ui->decayIndexInsertFn, &QPushButton::clicked, this, &FunctionGenerator::decayIndexFunButtonClicked);
    connect(ui->decayBaseEdit, &QLineEdit::textChanged, this, &FunctionGenerator::decayEntryChanged);
    connect(ui->decayRateEdit, &QLineEdit::textChanged, this, &FunctionGenerator::decayEntryChanged);
    connect(ui->decayIndexEdit, &QLineEdit::textChanged, this, &FunctionGenerator::decayEntryChanged);
    connect(ui->decayTypeExponential, &QRadioButton::toggled, this, &FunctionGenerator::decayEntryChanged);
    connect(ui->decayTypeLinear, &QRadioButton::toggled, this, &FunctionGenerator::decayEntryChanged);

    // Inverse Signals
    connect(ui->numToInverseInsertFn, &QPushButton::clicked, this, &FunctionGenerator::inverseFunButtonClicked);
    connect(ui->numToInverseEdit, &QLineEdit::textChanged, this, &FunctionGenerator::inverseEntryChanged);

    // Markov Signals
    connect(ui->markovEdit, &QLineEdit::textChanged, this, &FunctionGenerator::markovEntryChanged);

    // LN Signals
    connect(ui->lnInsertFn, &QPushButton::clicked, this, &FunctionGenerator::LNFunButtonClicked);
    connect(ui->lnEdit, &QLineEdit::textChanged, this, &FunctionGenerator::LNEntryChanged);

    // Fibonacci Signals
    connect(ui->fibInsertFn, &QPushButton::clicked, this, &FunctionGenerator::FibonacciFunButtonClicked);
    connect(ui->fibEdit, &QLineEdit::textChanged, this, &FunctionGenerator::FibonacciEntryChanged);

    // Select Signals
    connect(ui->selectIndexInsertFn, &QPushButton::clicked, this, &FunctionGenerator::selectIndexFunButtonClicked);
    connect(ui->selectIndexEdit, &QLineEdit::textChanged, this, &FunctionGenerator::selectTextChanged);
    connect(ui->selectAddNode, &QPushButton::clicked, this, &FunctionGenerator::addSelectNodeButtonClicked);
    connect(ui->selectInsertFn, &QPushButton::clicked, this, &FunctionGenerator::selectFunButtonClicked);

    // Stochos Signals
    connect(ui->stochosOffsetEdit, &QLineEdit::textChanged, this, &FunctionGenerator::stochosTextChanged);
    connect(ui->stochosMethodRange, &QRadioButton::toggled, this, &FunctionGenerator::stochosTextChanged);
    connect(ui->stochosMethodFunction, &QRadioButton::toggled, this, &FunctionGenerator::stochosTextChanged);
    connect(ui->stochosAddNode, &QPushButton::clicked, this, &FunctionGenerator::addStochosNodeButtonClicked);
    connect(ui->stochosMethodRange, &QRadioButton::toggled, this, &FunctionGenerator::clearStochosNodes);
    connect(ui->stochosMethodFunction, &QRadioButton::toggled, this, &FunctionGenerator::clearStochosNodes);
    connect(ui->stochosOffsetEdit, &QLineEdit::cursorPositionChanged, this, [this](){ this->m_lastFocusedStochosEdit = ui->stochosOffsetEdit; });
    connect(ui->stochosInsertFn, &QPushButton::clicked, this, &FunctionGenerator::stochosFunButtonClicked);

    // ValuePick Signals
    connect(ui->valuePickElementsMeaningful, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickElementsMods, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickElementsFake, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickElementsFib, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickWeightsPeriodic, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickWeightsHierarchic, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickWeightsInclude, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickTypeVariable, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickTypeConstant, &QRadioButton::toggled, this, &FunctionGenerator::valuePickTextChanged);

    connect(ui->valuePickAbsRangeEdit, &QLineEdit::textChanged, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickLowEdit, &QLineEdit::textChanged, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickHighEdit, &QLineEdit::textChanged, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickDistEdit, &QLineEdit::textChanged, this, &FunctionGenerator::valuePickTextChanged);

    connect(ui->valuePickValuesElementsEdit, &QLineEdit::textChanged, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickOffsetEdit, &QLineEdit::textChanged, this, &FunctionGenerator::valuePickTextChanged);
    connect(ui->valuePickValuesWeightsEdit, &QLineEdit::textChanged, this, &FunctionGenerator::valuePickTextChanged);

    connect(ui->valuePickAbsRangeInsertFn, &QPushButton::clicked, this, &FunctionGenerator::valuePickAbsRangeFunButtonClicked);
    connect(ui->valuePickLowInsertFn, &QPushButton::clicked, this, &FunctionGenerator::valuePickLowFunButtonClicked);
    connect(ui->valuePickHighInsertFn, &QPushButton::clicked, this, &FunctionGenerator::valuePickHighFunButtonClicked);
    connect(ui->valuePickDistInsertFn, &QPushButton::clicked, this, &FunctionGenerator::valuePickDistFunButtonClicked);

    // ChooseL Signals
    connect(ui->chooseLInsertFn, &QPushButton::clicked, this, &FunctionGenerator::chooseLFunButtonClicked);
    connect(ui->chooseLEdit, &QLineEdit::textChanged, this, &FunctionGenerator::chooseLEntryChanged);

    // GetPattern Signals
    connect(ui->getPatternMethodInOrder, &QRadioButton::toggled, this, &FunctionGenerator::getPatternEntryChanged);
    connect(ui->getPatternMethodOther, &QRadioButton::toggled, this, &FunctionGenerator::getPatternEntryChanged);
    connect(ui->getPatternMethodTypeClusters, &QRadioButton::toggled, this, &FunctionGenerator::getPatternEntryChanged);
    connect(ui->getPatternMethodTimeDepend, &QRadioButton::toggled, this, &FunctionGenerator::getPatternEntryChanged);
    connect(ui->getPatternMethodProbability, &QRadioButton::toggled, this, &FunctionGenerator::getPatternEntryChanged);

    connect(ui->getPatternOriginInsertFn, &QPushButton::clicked, this, &FunctionGenerator::getPatternOffsetFunButtonClicked);
    connect(ui->getPatternChooseInsertFn, &QPushButton::clicked, this, &FunctionGenerator::getPatternFunButtonClicked);

    connect(ui->getPatternOriginEdit, &QLineEdit::textChanged, this, &FunctionGenerator::getPatternEntryChanged);
    connect(ui->getPatternChooseEdit, &QLineEdit::textChanged, this, &FunctionGenerator::getPatternEntryChanged);

    // MakeList Signals
    connect(ui->makeListFunctionInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeListFunctionFunButtonClicked);
    connect(ui->makeListSizeInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeListSizeFunButtonClicked);
    connect(ui->makeListFunctionEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeListTextChanged);
    connect(ui->makeListSizeEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeListTextChanged);

    // EnvLib Signals
    connect(ui->envLibNumInsertFn, &QPushButton::clicked, this, &FunctionGenerator::envLibEnvelopeFunButtonClicked);
    connect(ui->envLibScalingInsertFn, &QPushButton::clicked, this, &FunctionGenerator::envLibScalingFactorFunButtonClicked);
    connect(ui->envLibNumEdit, &QLineEdit::textChanged, this, &FunctionGenerator::envLibTextChanged);
    connect(ui->envLibScalingEdit, &QLineEdit::textChanged, this, &FunctionGenerator::envLibTextChanged);

    // ReadENVFile Signals
    connect(ui->readEnvFileEdit, &QLineEdit::textChanged, this, &FunctionGenerator::readENVFileTextChanged);

    // MakeEnvelope Signals
    connect(ui->makeEnvelopeScalingInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeEnvelopeScalingFactorFunButtonClicked);
    connect(ui->makeEnvelopeXInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeEnvelopeXValueFunButtonClicked);
    connect(ui->makeEnvelopeYInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeEnvelopeYValueFunButtonClicked);
    connect(ui->makeEnvelopeScalingEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeEnvelopeTextChanged);
    connect(ui->makeEnvelopeXEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeEnvelopeTextChanged);
    connect(ui->makeEnvelopeYEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeEnvelopeTextChanged);

    // MakePattern Signals
    connect(ui->makePatternIntervalsInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makePatternIntervalsFunButtonClicked);
    connect(ui->makePatternIntervalsEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makePatternTextChanged);

    // ExpandPattern Signals
    connect(ui->expandPatternMethodEquivalence, &QRadioButton::toggled, this, &FunctionGenerator::expandPatternTextChanged);
    connect(ui->expandPatternMethodSymmetries, &QRadioButton::toggled, this, &FunctionGenerator::expandPatternTextChanged);
    connect(ui->expandPatternMethodDistort, &QRadioButton::toggled, this, &FunctionGenerator::expandPatternTextChanged);

    connect(ui->expandPatternModuloInsertFn, &QPushButton::clicked, this, &FunctionGenerator::expandPatternModuloFunButtonClicked);
    connect(ui->expandPatternLowInsertFn, &QPushButton::clicked, this, &FunctionGenerator::expandPatternLowFunButtonClicked);
    connect(ui->expandPatternHighInsertFn, &QPushButton::clicked, this, &FunctionGenerator::expandPatternHighFunButtonClicked);
    connect(ui->expandPatternPatternInsertFn, &QPushButton::clicked, this, &FunctionGenerator::expandPatternPatternFunButtonClicked);

    connect(ui->expandPatternModuloEdit, &QLineEdit::textChanged, this, &FunctionGenerator::expandPatternTextChanged);
    connect(ui->expandPatternLowEdit, &QLineEdit::textChanged, this, &FunctionGenerator::expandPatternTextChanged);
    connect(ui->expandPatternHighEdit, &QLineEdit::textChanged, this, &FunctionGenerator::expandPatternTextChanged);
    connect(ui->expandPatternPatternEdit, &QLineEdit::textChanged, this, &FunctionGenerator::expandPatternTextChanged);

    // ReadPATFile Signals
    connect(ui->readPatFileEdit, &QLineEdit::textChanged, this, &FunctionGenerator::readPATFileTextChanged);

    // ReadFILFile Signals
    connect(ui->readFilFileEdit, &QLineEdit::textChanged, this, &FunctionGenerator::readFILFileTextChanged);

    // MakeFilter Signals
    connect(ui->makeFilterLPF, &QRadioButton::toggled, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterHPF, &QRadioButton::toggled, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterBPF, &QRadioButton::toggled, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterLSF, &QRadioButton::toggled, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterHSF, &QRadioButton::toggled, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterNF, &QRadioButton::toggled, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterPBEQ, &QRadioButton::toggled, this, &FunctionGenerator::makeFilterTextChanged);

    connect(ui->makeFilterFrequencyInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeFilterFrequencyFunButtonClicked);
    connect(ui->makeFilterBWInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeFilterBandWidthFunButtonClicked);
    connect(ui->makeFilterDBInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeFilterDBGainFunButtonClicked);

    connect(ui->makeFilterFrequencyEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterBWEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeFilterTextChanged);
    connect(ui->makeFilterDBEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeFilterTextChanged);

    // ReadREVFile Signals
    connect(ui->readRevFileEdit, &QLineEdit::textChanged, this, &FunctionGenerator::readREVFileTextChanged);
    
    // REV signals
    REVMethodFlag = 0; // default to REV_Simple
    connect(ui->revApplySound, &QRadioButton::toggled, this, &FunctionGenerator::handleRevApplyMethodChanged);
    connect(ui->revApplyPartial, &QRadioButton::toggled, this, &FunctionGenerator::handleRevApplyMethodChanged);

    // ReadSIVFile Signals
    connect(ui->readSivFileEdit, &QLineEdit::textChanged, this, &FunctionGenerator::readSIVFileTextChanged);

    // MakeSieve Signals
    connect(ui->makeSieveElementsMeaningful, &QRadioButton::toggled, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveElementsMods, &QRadioButton::toggled, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveElementsFake, &QRadioButton::toggled, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveElementsFib, &QRadioButton::toggled, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveWeightsPeriodic, &QRadioButton::toggled, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveWeightsHierarchic, &QRadioButton::toggled, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveWeightsInclude, &QRadioButton::toggled, this, &FunctionGenerator::makeSieveTextChanged);

    connect(ui->makeSieveLowBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeSieveLowFunButtonClicked);
    connect(ui->makeSieveHighBoundInsertFn, &QPushButton::clicked, this, &FunctionGenerator::makeSieveHighFunButtonClicked);

    connect(ui->makeSieveLowBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveHighBoundEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveElementsValuesEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveOffsetEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeSieveTextChanged);
    connect(ui->makeSieveWeightsValuesEdit, &QLineEdit::textChanged, this, &FunctionGenerator::makeSieveTextChanged);

    // ReadSPAFile Signals
    connect(ui->readSpaFileEdit, &QLineEdit::textChanged, this, &FunctionGenerator::readSPAFileTextChanged);

    // SPA Signals
    connect(ui->spaStereo, &QRadioButton::toggled, this, &FunctionGenerator::handleSpaApplyMethodChanged);
    connect(ui->spaMultiPan, &QRadioButton::toggled, this, &FunctionGenerator::handleSpaApplyMethodChanged);
    connect(ui->spaPolar, &QRadioButton::toggled, this, &FunctionGenerator::handleSpaApplyMethodChanged);
    connect(ui->spaApplySound, &QRadioButton::toggled, this, &FunctionGenerator::handleSpaApplyMethodChanged);
    connect(ui->spaApplyPartial, &QRadioButton::toggled, this, &FunctionGenerator::handleSpaApplyMethodChanged);

    // SpectrumGen Signals
    connect(ui->spectrumGenEnvelopeInsertFn, &QPushButton::clicked, this, &FunctionGenerator::Spectrum_GenEnvelopeFunButtonClicked);
    connect(ui->spectrumGenDistanceInsertFn, &QPushButton::clicked, this, &FunctionGenerator::Spectrum_GenDistanceFunButtonClicked);
    connect(ui->spectrumGenEnvelopeEdit, &QLineEdit::textChanged, this, &FunctionGenerator::Spectrum_GenTextChanged);
    connect(ui->spectrumGenDistanceEdit, &QLineEdit::textChanged, this, &FunctionGenerator::Spectrum_GenTextChanged);

    // Helper: find and select the combo box item matching the function name
    auto selectComboItem = [&](const QString& name) {
        for (int i = 0; i < ui->functionOptions->count(); ++i) {
            if (ui->functionOptions->itemText(i) == name) {
                ui->functionOptions->setCurrentIndex(i);
                return;
            }
        }
    };

    if (originalString.isEmpty()) return;

    QXmlStreamReader r(originalString);
    if (!r.readNextStartElement()) return;       // <Fun> root
    if (!r.readNextStartElement()) return;       // <Name>
    const QString functionName = readInner(r);   // cursor now at </Name>

    // Hybrid-mode dispatch: registered widgets handle their own parsing.
    const CMODFunction registeredId =
        FunctionRegistry::instance().idFromXmlName(functionName);
    if (registeredId != NOT_A_FUNCTION) {
        if (FunctionWidget* w = ensureRegisteredWidget(registeredId)) {
            selectComboItem(functionName);
            w->populateFromXML(r);
            return;
        }
    }

    if (functionName == "RandomInt") {
        selectComboItem(functionName);
        ui->randomIntLowerBoundEdit->setText(nextChildInner(r));
        ui->randomIntUpperBoundEdit->setText(nextChildInner(r));
    } else if (functionName == "MakeSieve") {
        selectComboItem(functionName);
        ui->makeSieveLowBoundEdit->setText(nextChildInner(r));
        ui->makeSieveHighBoundEdit->setText(nextChildInner(r));
        const QString methodStr = nextChildInner(r);
        if (methodStr == "MODS") ui->makeSieveElementsMods->setChecked(true);
        else if (methodStr == "FAKE") ui->makeSieveElementsFake->setChecked(true);
        else if (methodStr == "FIBONACCI") ui->makeSieveElementsFib->setChecked(true);
        else ui->makeSieveElementsMeaningful->setChecked(true); // default / MEANINGFUL
        ui->makeSieveElementsValuesEdit->setText(nextChildInner(r));
        const QString weightMethodStr = nextChildInner(r);
        if (weightMethodStr == "HIERARCHIC") ui->makeSieveWeightsHierarchic->setChecked(true);
        else if (weightMethodStr == "INCLUDE") ui->makeSieveWeightsInclude->setChecked(true);
        else ui->makeSieveWeightsPeriodic->setChecked(true); // default / PERIODIC
        ui->makeSieveWeightsValuesEdit->setText(nextChildInner(r));
        ui->makeSieveOffsetEdit->setText(nextChildInner(r));
    }
    else if (functionName == "MakeFilter") {
        selectComboItem(functionName);
        const QString filterType = nextChildInner(r);
        if (filterType == "HPF") ui->makeFilterHPF->setChecked(true);
        else if (filterType == "BPF") ui->makeFilterBPF->setChecked(true);
        else if (filterType == "NF") ui->makeFilterNF->setChecked(true);
        else if (filterType == "PBEQF") ui->makeFilterPBEQ->setChecked(true);
        else if (filterType == "LSF") ui->makeFilterLSF->setChecked(true);
        else if (filterType == "HSF") ui->makeFilterHSF->setChecked(true);
        else ui->makeFilterLPF->setChecked(true); // default / LPF
        ui->makeFilterFrequencyEdit->setText(nextChildInner(r));
        ui->makeFilterBWEdit->setText(nextChildInner(r));
        ui->makeFilterDBEdit->setText(nextChildInner(r));
    }
    else if (functionName == "ValuePick") {
        selectComboItem(functionName);
        ui->valuePickAbsRangeEdit->setText(nextChildInner(r));
        ui->valuePickLowEdit->setText(nextChildInner(r));
        ui->valuePickHighEdit->setText(nextChildInner(r));
        ui->valuePickDistEdit->setText(nextChildInner(r));
        const QString method = nextChildInner(r);
        if (method == "MEANINGFUL") ui->valuePickElementsMeaningful->setChecked(true);
        else if (method == "MODS") ui->valuePickElementsMods->setChecked(true);
        else if (method == "FAKE") ui->valuePickElementsFake->setChecked(true);
        else if (method == "FIBONACCI") ui->valuePickElementsFib->setChecked(true);
        ui->valuePickValuesElementsEdit->setText(nextChildInner(r));
        const QString weightMethod = nextChildInner(r);
        if (weightMethod == "PERIODIC") ui->valuePickWeightsPeriodic->setChecked(true);
        else if (weightMethod == "HIERARCHIC") ui->valuePickWeightsHierarchic->setChecked(true);
        else if (weightMethod == "INCLUDE") ui->valuePickWeightsInclude->setChecked(true);
        ui->valuePickValuesWeightsEdit->setText(nextChildInner(r));
        const QString type = nextChildInner(r);
        if (type == "VARIABLE") ui->valuePickTypeVariable->setChecked(true);
        else if (type == "CONSTANT") ui->valuePickTypeConstant->setChecked(true);
        ui->valuePickOffsetEdit->setText(nextChildInner(r));
    } else if (functionName == "GetPattern") {
        selectComboItem(functionName);
        const QString method = nextChildInner(r);
        if (method == "OTHER") ui->getPatternMethodOther->setChecked(true);
        else if (method == "TYPE_CLUSTERS") ui->getPatternMethodTypeClusters->setChecked(true);
        else if (method == "TIME_DEPEND") ui->getPatternMethodTimeDepend->setChecked(true);
        else if (method == "PROBABILITY") ui->getPatternMethodProbability->setChecked(true);
        else ui->getPatternMethodInOrder->setChecked(true); // default / IN_ORDER
        ui->getPatternOriginEdit->setText(nextChildInner(r));
        ui->getPatternChooseEdit->setText(nextChildInner(r));
    } else if (functionName == "MakePattern") {
        selectComboItem(functionName);
        ui->makePatternIntervalsEdit->setText(nextChildInner(r));
    } else if (functionName == "ExpandPattern") {
        selectComboItem(functionName);
        const QString method = nextChildInner(r);
        if (method == "SYMMETRIES") ui->expandPatternMethodSymmetries->setChecked(true);
        else if (method == "DISTORT") ui->expandPatternMethodDistort->setChecked(true);
        else ui->expandPatternMethodEquivalence->setChecked(true); // default / EQUIVALENCE
        ui->expandPatternModuloEdit->setText(nextChildInner(r));
        ui->expandPatternLowEdit->setText(nextChildInner(r));
        ui->expandPatternHighEdit->setText(nextChildInner(r));
        ui->expandPatternPatternEdit->setText(nextChildInner(r));
    } else if (functionName == "ReadPATFile") {
        selectComboItem(functionName);
        ui->readPatFileEdit->setText(nextChildInner(r));
    } else if (functionName == "ReadSIVFile") {
        selectComboItem(functionName);
        ui->readSivFileEdit->setText(nextChildInner(r));
    } else if (functionName == "ReadENVFile") {
        selectComboItem(functionName);
        ui->readEnvFileEdit->setText(nextChildInner(r));
    } else if (functionName == "ReadFILFile") {
        selectComboItem(functionName);
        ui->readFilFileEdit->setText(nextChildInner(r));
    } else if (functionName == "ChooseL") {
        selectComboItem(functionName);
        ui->chooseLEdit->setText(nextChildInner(r));
    } else if (functionName == "EnvLib") {
        selectComboItem(functionName);
        ui->envLibNumEdit->setText(nextChildInner(r));
        ui->envLibScalingEdit->setText(nextChildInner(r));
    } else if (functionName == "Select") {
        selectComboItem("Select");

        // Clear any default nodes added by handleFunctionChanged
        while (ui->selectScrollLayout->count() > 1) {
            QLayoutItem* item = ui->selectScrollLayout->itemAt(0);
            if (item && item->widget()) {
                Select* node = qobject_cast<Select*>(item->widget());
                if (node) { removeSelectNodeButtonClicked(node); }
                else break;
            } else break;
        }

        // Read inner XML of <List>, then <Index>
        QString listContentQ;
        if (r.readNextStartElement()) // <List>
            listContentQ = readInner(r);
        std::string listContent = listContentQ.toStdString();

        // Split by top-level commas (skip commas inside <Fun>…</Fun>)
        std::vector<std::string> selectItems;
        if (!listContent.empty()) {
            std::string rem = listContent;
            while (!rem.empty()) {
                size_t cp = rem.find(',');
                size_t fp = rem.find("<Fun>");
                while (fp != std::string::npos && cp != std::string::npos && fp < cp) {
                    int depth = 1;
                    size_t pos = fp + 5;
                    while (depth > 0 && pos < rem.size()) {
                        size_t nf = rem.find("<Fun>", pos);
                        size_t ne = rem.find("</Fun>", pos);
                        if (nf != std::string::npos && (ne == std::string::npos || nf < ne)) {
                            depth++; pos = nf + 5;
                        } else if (ne != std::string::npos) {
                            depth--; pos = ne + 6;
                        } else break;
                    }
                    cp = rem.find(',', pos);
                    fp = rem.find("<Fun>", pos);
                }
                if (cp == std::string::npos) { selectItems.push_back(rem); break; }
                selectItems.push_back(rem.substr(0, cp));
                rem = rem.substr(cp + 1);
            }
        }

        for (const std::string& itemStr : selectItems) {
            std::string trimmed = itemStr;
            if (!trimmed.empty() && trimmed.front() == ' ') trimmed = trimmed.substr(1);
            if (!trimmed.empty()) {
                addSelectNodeButtonClicked();
                int last = ui->selectScrollLayout->count() - 2;
                QLayoutItem* item = ui->selectScrollLayout->itemAt(last);
                if (item && item->widget()) {
                    Select* node = qobject_cast<Select*>(item->widget());
                    if (node) node->setValText(QString::fromStdString(trimmed));
                }
            }
        }

        ui->selectIndexEdit->setText(nextChildInner(r)); // <Index>
    } else if (functionName == "Stochos") {
        selectComboItem("Stochos");
        // selectComboItem triggers handleFunctionChanged which sets RANGE_DISTRIB and clears nodes

        const QString method = nextChildInner(r); // <Method>
        bool isRange = (method != "FUNCTIONS");
        if (!isRange) {
            // Switching to FUNCTIONS triggers clearStochosNodes (harmless, no nodes yet)
            ui->stochosMethodFunction->setChecked(true);
        }

        // Iterate over <Envelope> children of <Envelopes>
        if (r.readNextStartElement()) { // <Envelopes>
            while (r.readNextStartElement()) {
                addStochosNodeButtonClicked();
                int last = ui->stochosScrollLayout->count() - 2;
                QLayoutItem* item = ui->stochosScrollLayout->itemAt(last);
                Stochos* node = item ? qobject_cast<Stochos*>(item->widget()) : nullptr;
                if (!node) { r.skipCurrentElement(); break; }

                if (isRange) {
                    // Three <Envelope> elements per row: min, max, dist
                    node->setMinText(readInner(r));
                    if (r.readNextStartElement()) node->setMaxText(readInner(r));
                    if (r.readNextStartElement()) node->setDistText(readInner(r));
                } else {
                    // One <Envelope> per row
                    node->setValText(readInner(r));
                }
            }
        }

        ui->stochosOffsetEdit->setText(nextChildInner(r)); // <Offset>
    } else if (functionName == "Markov") {
        selectComboItem("GetFromMarkovChain");
        ui->markovEdit->setText(nextChildInner(r));
    } else if (functionName == "REV_Simple" || functionName == "REV_Medium" || functionName == "REV_Advanced") {
        selectComboItem(functionName);
        // selectComboItem → handleFunctionChanged → sets REVMethodFlag, creates 1 default SOUND channel

        const QString applyMethod = nextChildInner(r); // <Apply>

        // Block signals to prevent handleRevApplyMethodChanged from firing twice
        // (once per radio button toggling) and creating orphaned layout widgets.
        {
            QSignalBlocker sb1(ui->revApplySound);
            QSignalBlocker sb2(ui->revApplyPartial);
            if (applyMethod == "PARTIAL") {
                ui->revApplyPartial->setChecked(true);
                ui->revApplySound->setChecked(false);
            }
        }

        // Discard the default SOUND channel; we'll build fresh from the XML data.
        clearRevChannels();

        if (functionName == "REV_Simple") {
            // <Sizes><Size>...</Size>...</Sizes>
            if (r.readNextStartElement()) {
                while (r.readNextStartElement()) {
                    REVChannel* chan = REVInsertChannel(m_revChannels.isEmpty() ? nullptr : m_revChannels.last());
                    chan->setRoomSizeText(readInner(r));
                }
            }
        } else if (functionName == "REV_Medium") {
            // <Percents>, <Spreads>, <AllPasses>, <Delays>
            QStringList percents, spreads, allpasses, delays;
            auto readList = [&](QStringList& out) {
                if (!r.readNextStartElement()) return;
                while (r.readNextStartElement())
                    out.append(readInner(r));
            };
            readList(percents);
            readList(spreads);
            readList(allpasses);
            readList(delays);

            for (int i = 0; i < percents.size(); ++i) {
                REVChannel* chan = REVInsertChannel(m_revChannels.isEmpty() ? nullptr : m_revChannels.last());
                chan->setReverbText(percents[i]);
                if (i < spreads.size())   chan->setHillowText(spreads[i]);
                if (i < allpasses.size()) chan->setAllGainText(allpasses[i]);
                if (i < delays.size())    chan->setDelayText(delays[i]);
            }
        } else { // REV_Advanced
            // <Percents>, <CombGainLists>, <LPGainLists>, <AllPasses>, <Delays>
            QStringList percents, combs, lps, allpasses, delays;
            auto readList = [&](QStringList& out) {
                if (!r.readNextStartElement()) return;
                while (r.readNextStartElement())
                    out.append(readInner(r));
            };
            readList(percents);
            readList(combs);
            readList(lps);
            readList(allpasses);
            readList(delays);

            for (int i = 0; i < percents.size(); ++i) {
                REVChannel* chan = REVInsertChannel(m_revChannels.isEmpty() ? nullptr : m_revChannels.last());
                chan->setReverbText(percents[i]);
                if (i < combs.size())     chan->setCombGainText(combs[i]);
                if (i < lps.size())       chan->setLPGainText(lps[i]);
                if (i < allpasses.size()) chan->setAllGainText(allpasses[i]);
                if (i < delays.size())    chan->setDelayText(delays[i]);
            }
        }

        // Restore SOUND-mode presentation (title + hidden buttons) if applicable.
        if (applyMethod != "PARTIAL" && !m_revChannels.isEmpty()) {
            m_revChannels.first()->setTitle("Sound");
            m_revChannels.first()->hideButtons();
        }
    } else if (functionName == "ReadREVFile") {
        selectComboItem(functionName);
        ui->readRevFileEdit->setText(nextChildInner(r));
    } else if (functionName == "SPA") {
        const QString method = nextChildInner(r); // <Method>
        const QString apply  = nextChildInner(r); // <Apply>

        // Block signals BEFORE selectComboItem so that handleFunctionChanged's
        // setChecked(true) calls don't emit toggled and trigger
        // handleSpaApplyMethodChanged as side effects.
        ui->spaStereo->blockSignals(true);
        ui->spaMultiPan->blockSignals(true);
        ui->spaPolar->blockSignals(true);
        ui->spaApplySound->blockSignals(true);
        ui->spaApplyPartial->blockSignals(true);

        selectComboItem("SPA");
        // handleFunctionChanged still makes one explicit handleSpaApplyMethodChanged()
        // call, which creates one default channel. Delete it immediately (not via
        // deleteLater) so no ghost channel lingers in the layout.
        for (SPAChannel* chan : m_spaChannels) {
            ui->spaScrollWindowLayout->removeWidget(chan);
            delete chan;
        }
        m_spaChannels.clear();

        if (method == "MULTI_PAN") ui->spaMultiPan->setChecked(true);
        else if (method == "POLAR") ui->spaPolar->setChecked(true);
        else                        ui->spaStereo->setChecked(true);

        if (apply == "PARTIAL") ui->spaApplyPartial->setChecked(true);
        else                    ui->spaApplySound->setChecked(true);

        ui->spaStereo->blockSignals(false);
        ui->spaMultiPan->blockSignals(false);
        ui->spaPolar->blockSignals(false);
        ui->spaApplySound->blockSignals(false);
        ui->spaApplyPartial->blockSignals(false);

        handleSpaApplyMethodChanged();

        // Populate channels from <Channels><Partials><P>…</P>…</Partials>…</Channels>
        if (r.readNextStartElement()) { // <Channels>
            int chanIdx = 0;
            while (r.readNextStartElement()) { // <Partials>
                while (chanIdx >= m_spaChannels.size())
                    SPAInsertChannel(m_spaChannels.isEmpty() ? nullptr : m_spaChannels.last());
                SPAChannel* chan = m_spaChannels[chanIdx];

                int rowIdx = 0;
                while (r.readNextStartElement()) { // <P>
                    while (rowIdx >= chan->rowCount())
                        chan->addRow(chan->rowCount(), true);
                    chan->setRowText(rowIdx, readInner(r));
                    rowIdx++;
                }
                chanIdx++;
            }
        }
    } else if (functionName == "ReadSPAFile") {
        selectComboItem("ReadSPAFile");
        ui->readSpaFileEdit->setText(nextChildInner(r));
    } else if (functionName == "RandomOrderInt") {
        selectComboItem(functionName);
        ui->randomOrderIntLowerBoundEdit->setText(nextChildInner(r));
        ui->randomOrderIntUpperBoundEdit->setText(nextChildInner(r));
    } else if (functionName == "Random") {
        selectComboItem(functionName);
        ui->randomLowerBoundEdit->setText(nextChildInner(r));
        ui->randomUpperBoundEdit->setText(nextChildInner(r));
    } else if (functionName == "Randomizer") {
        selectComboItem(functionName);
        ui->randomizerBaseEdit->setText(nextChildInner(r));
        ui->randomizerDeviationEdit->setText(nextChildInner(r));
    } else if (functionName == "RandomDensity") {
        selectComboItem(functionName);
        ui->randomDensityEnvEdit->setText(nextChildInner(r));
        ui->randomDensityStartTimeEdit->setText(nextChildInner(r));
        ui->randomDensityEndTimeEdit->setText(nextChildInner(r));
    } else if (functionName == "Inverse") {
        selectComboItem(functionName);
        ui->numToInverseEdit->setText(nextChildInner(r));
    } else if (functionName == "LN") {
        selectComboItem(functionName);
        ui->lnEdit->setText(nextChildInner(r));
    } else if (functionName == "Fibonacci") {
        selectComboItem(functionName);
        ui->fibEdit->setText(nextChildInner(r));
    }
    //not work: Decay
}

void FunctionGenerator::handleFunctionChanged(int index)
{
    // Gets the function id name of the current index the combo box has selected
    QVariant data = ui->functionOptions->itemData(index);
    if (!data.isValid()) return;
    // Converts the function id name into an integer
    CMODFunction functionId = static_cast<CMODFunction>(data.toInt());

    // Hybrid-mode dispatch: a registered FunctionWidget owns its own page.
    if (FunctionRegistry::instance().isRegistered(functionId)) {
        if (FunctionWidget* w = ensureRegisteredWidget(functionId)) {
            ui->functionStackedWidget->setCurrentIndex(m_registeredPageIndex.value(functionId));
            ui->resultTextEdit->setText(w->buildXMLString());
            resizeToFitCurrentPage();
            return;
        }
    }

    /* Case statements used to set the stacked widget current index (new form widget added)
     based on the function id */
    int currPageIndex = 0;
    // initializes variables used
    int oldREVMethodFlag = REVMethodFlag;
    switch (functionId) {
        case NOT_A_FUNCTION:
        case functionRawList:
            ui->resultTextEdit->setText("");
            currPageIndex = 0;
            break;
        case function_staticCURRENT_TYPE:
            ui->resultTextEdit->setText("<Fun><Name>CURRENT_TYPE</Name></Fun>");
            currPageIndex = 0;
            break;
        case function_staticCURRENT_CHILD_NUM:
            ui->resultTextEdit->setText("<Fun><Name>CURRENT_CHILD_NUM</Name></Fun>");
            currPageIndex = 0;
            break;
        case function_staticCURRENT_PARTIAL_NUM:
            ui->resultTextEdit->setText("<Fun><Name>CURRENT_PARTIAL_NUM</Name></Fun>");
            currPageIndex = 0;
            break;
        case function_staticCURRENT_SEGMENT:
            ui->resultTextEdit->setText("<Fun><Name>CURRENT_SEGMENT</Name></Fun>");
            currPageIndex = 0;
            break;
        case function_staticAVAILABLE_EDU:
            ui->resultTextEdit->setText("<Fun><Name>AVAILABLE_EDU</Name></Fun>");
            currPageIndex = 0;
            break;
        case function_staticPREVIOUS_CHILD_DURATION:
            ui->resultTextEdit->setText("<Fun><Name>PREVIOUS_CHILD_DURATION</Name></Fun>");
            currPageIndex = 0;
            break;
        case function_staticCURRENT_LAYER:
            ui->resultTextEdit->setText("<Fun><Name>CURRENT_LAYER</Name></Fun>");
            currPageIndex = 0;
            break;
        case functionRandom:
            currPageIndex = 1;
            ui->randomLowerBoundEdit->setText("0");
            ui->randomUpperBoundEdit->setText("1");
            randomEntryChanged();
            break;
        case functionRandomInt:
            currPageIndex = 2;
            ui->randomIntLowerBoundEdit->setText("0");
            ui->randomIntUpperBoundEdit->setText("1");
            randomIntEntryChanged();
            break;
        case functionRandomOrderInt:
            currPageIndex = 3;
            ui->randomOrderIntLowerBoundEdit->setText("0");
            ui->randomOrderIntUpperBoundEdit->setText("1");
            randomOrderIntEntryChanged();
            break;
        case functionRandomizer:
            currPageIndex = 4;
            randomizerEntryChanged();
            break;
        case functionRandomDensity:
            currPageIndex = 5;
            randomDensityEntryChanged();
            break;
        case functionInverse:
            currPageIndex = 6; 
            inverseEntryChanged();
            break;
        case functionLN:
            currPageIndex = 7;
            LNEntryChanged();
            break;
        case functionFibonacci:
            currPageIndex = 8;
            FibonacciEntryChanged();
            break;
        case functionDecay:
            currPageIndex = 9;
            ui->decayIndexEdit->clear();
            ui->decayIndexEdit->setPlaceholderText("CURRENT_PARTIAL_NUM");
            ui->decayTypeExponential->setChecked(true);
            decayEntryChanged();
            break;
        case functionMarkov:
            currPageIndex = 10;
            markovEntryChanged();
            break;
        case functionStochos:
            currPageIndex = 11;
            ui->stochosScrollLayout->setSpacing(0);
            ui->stochosScrollLayout->setContentsMargins(0, 0, 0, 0);
            ui->stochosMethodRange->setChecked(true);
            stochosTextChanged();
            break;
        case functionSelect:
            currPageIndex = 12;
            ui->selectScrollLayout->setSpacing(0);
            ui->selectScrollLayout->setContentsMargins(0, 0, 0, 0);
            selectTextChanged();
            break;
        case functionValuePick:
            currPageIndex = 13;
            ui->valuePickAbsRangeEdit->setText("1");
            ui->valuePickLowEdit->clear();
            ui->valuePickLowEdit->setPlaceholderText("ENV");
            ui->valuePickHighEdit->clear();
            ui->valuePickHighEdit->setPlaceholderText("ENV");
            ui->valuePickDistEdit->clear();
            ui->valuePickDistEdit->setPlaceholderText("ENV");
            ui->valuePickElementsMeaningful->setChecked(true);
            ui->valuePickValuesElementsEdit->clear();
            ui->valuePickValuesElementsEdit->setPlaceholderText("INT1, INT2, INT3 ...");
            ui->valuePickWeightsPeriodic->setChecked(true);
            ui->valuePickValuesWeightsEdit->clear();
            ui->valuePickValuesWeightsEdit->setPlaceholderText("INT1, INT2, INT3 ...");
            ui->valuePickTypeVariable->setChecked(true);
            ui->valuePickOffsetEdit->clear();
            ui->valuePickOffsetEdit->setPlaceholderText("INT1, INT2, INT3 ...");
            valuePickTextChanged();
            break;
        case functionChooseL:
            currPageIndex = 14;
            ui->chooseLEdit->clear();
            ui->chooseLEdit->setPlaceholderText("SIV");
            chooseLEntryChanged();
            break;
        case functionGetPattern:
            currPageIndex = 15;
            ui->getPatternMethodInOrder->setChecked(true);
            ui->getPatternChooseEdit->clear();
            ui->getPatternChooseEdit->setPlaceholderText("PAT");
            getPatternEntryChanged();
            break;
        case functionEnvLib:
            currPageIndex = 16;
            ui->envLibScalingEdit->setText("1.0");
            envLibTextChanged();
            break;
        case functionMakeEnvelope:
            currPageIndex = 17;
            makeEnvelopeTextChanged();
            break;
        case functionReadENVFile:
            currPageIndex = 18;
            readENVFileTextChanged();
            break;
        case functionMakeList:
            currPageIndex = 19;
            makeListTextChanged();
            break;
        case functionSPA:
            currPageIndex = 20;
            ui->spaScrollWindowLayout->setSpacing(5);
            ui->spaScrollWindowLayout->setContentsMargins(5, 5, 5, 5);
            if (m_spaChannels.isEmpty()) {
                ui->spaStereo->setChecked(true);
                ui->spaApplySound->setChecked(true);
                handleSpaApplyMethodChanged();
            }
            SPATextChanged();
            break;
        case functionReadSPAFile:
            currPageIndex = 21;
            readSPAFileTextChanged();
            break;
        case functionMakeFilter:
            currPageIndex = 22;
            makeFilterTextChanged();
            break;
        case functionReadFILFile:
            currPageIndex = 23;
            readFILFileTextChanged();
            break;
        case functionMakePattern:
            currPageIndex = 24;
            ui->makePatternIntervalsEdit->clear();
            ui->makePatternIntervalsEdit->setPlaceholderText("INT1, INT2, INT3 ...");
            makePatternTextChanged();
            break;  
        case functionExpandPattern:
            currPageIndex = 25;
            ui->expandPatternMethodEquivalence->setChecked(true);
            expandPatternTextChanged();
            break; 
        case functionReadPATFile:
            currPageIndex = 26;
            readPATFileTextChanged();
            break; 
        case functionREV_Simple:
            REVMethodFlag = 0;
            currPageIndex = 27;
            ui->revScrollWindowLayout->setSpacing(5);
            ui->revScrollWindowLayout->setContentsMargins(5, 5, 5, 5);
            clearRevChannels();
            if (m_revChannels.isEmpty()) {
                ui->revApplySound->setChecked(true);
                handleRevApplyMethodChanged();
            }
            REVTextChanged();
            break;
        case functionREV_Medium:
            REVMethodFlag = 1;
            currPageIndex = 27;
            ui->revScrollWindowLayout->setSpacing(5);
            ui->revScrollWindowLayout->setContentsMargins(5, 5, 5, 5);
            clearRevChannels();
            if (m_revChannels.isEmpty()) {
                ui->revApplySound->setChecked(true);
                handleRevApplyMethodChanged();
            }
            REVTextChanged();
            break;
        case functionREV_Advanced:
            REVMethodFlag = 2;
            currPageIndex = 27;
            ui->revScrollWindowLayout->setSpacing(5);
            ui->revScrollWindowLayout->setContentsMargins(5, 5, 5, 5);
            clearRevChannels();
            if (m_revChannels.isEmpty()) {
                ui->revApplySound->setChecked(true);
                handleRevApplyMethodChanged();
            }
            REVTextChanged();
            break;
        case functionReadREVFile:
            currPageIndex = 28;
            readREVFileTextChanged();
            break; 
        case functionSpectrum_Gen:
            currPageIndex = 29;
            ui->spectrumGenEnvelopeEdit->clear();
            ui->spectrumGenEnvelopeEdit->setPlaceholderText("ENV");
            Spectrum_GenTextChanged();
            break;
        case functionMakeSieve:
            currPageIndex = 30;
            ui->makeSieveLowBoundEdit->clear();
            ui->makeSieveLowBoundEdit->setPlaceholderText("ENV");
            ui->makeSieveHighBoundEdit->clear();
            ui->makeSieveHighBoundEdit->setPlaceholderText("ENV");
            ui->makeSieveElementsMeaningful->setChecked(true);
            ui->makeSieveElementsValuesEdit->clear();
            ui->makeSieveElementsValuesEdit->setPlaceholderText("INT1, INT2, INT3 ...");
            ui->makeSieveOffsetEdit->clear();
            ui->makeSieveOffsetEdit->setPlaceholderText("INT1, INT2, INT3 ...");
            ui->makeSieveWeightsPeriodic->setChecked(true);
            ui->makeSieveWeightsValuesEdit->clear();
            ui->makeSieveWeightsValuesEdit->setPlaceholderText("INT1, INT2, INT3 ...");
            makeSieveTextChanged();
            break;
        case functionReadSIVFile:
            currPageIndex = 31;
            readSIVFileTextChanged();
            break;
        default: currPageIndex = 0; break;
    }
    ui->functionStackedWidget->setCurrentIndex(currPageIndex);
    resizeToFitCurrentPage();
}

QString FunctionGenerator::getResultString(){
    result = ui->resultTextEdit->toPlainText();
    return result;
}

/* Random Signal Functions */
void FunctionGenerator::randomLowBoundFunButtonClicked(){
  QString initialText = ui->randomLowerBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomLowerBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomHighBoundFunButtonClicked(){
  QString initialText = ui->randomUpperBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomUpperBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>Random</Name><Low>" + ui->randomLowerBoundEdit->text() +  "</Low><High>";
    stringbuffer = stringbuffer + ui->randomUpperBoundEdit->text() + "</High></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* RandomInt Signal Functions */
void FunctionGenerator::randomIntLowBoundFunButtonClicked(){
  QString initialText = ui->randomIntLowerBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomIntLowerBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomIntHighBoundFunButtonClicked(){
  QString initialText = ui->randomIntUpperBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomIntUpperBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomIntEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>RandomInt</Name><Low>" + ui->randomIntLowerBoundEdit->text() +  "</Low><High>";
    stringbuffer = stringbuffer + ui->randomIntUpperBoundEdit->text() + "</High></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* RandomOrderInt Signal Functions */
void FunctionGenerator::randomOrderIntLowBoundFunButtonClicked(){
  QString initialText = ui->randomOrderIntLowerBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomOrderIntLowerBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomOrderIntHighBoundFunButtonClicked(){
  QString initialText = ui->randomOrderIntUpperBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomOrderIntUpperBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomOrderIntEntryChanged(){
    QString ts = QString::number(QDateTime::currentSecsSinceEpoch());
    QString stringbuffer;
    stringbuffer = "<Fun><Name>RandomOrderInt</Name><Low>" + ui->randomOrderIntLowerBoundEdit->text() + "</Low><High>";
    stringbuffer = stringbuffer + ui->randomOrderIntUpperBoundEdit->text() + "</High><Id>" + ts + "</Id></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* Randomizer Signal Functions */
void FunctionGenerator::randomizerBaseFunButtonClicked(){
  QString initialText = ui->randomizerBaseEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomizerBaseEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomizerDeviationFunButtonClicked(){
  QString initialText = ui->randomizerDeviationEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->randomizerDeviationEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::randomizerEntryChanged(){
    QString stringbuffer;
    stringbuffer =  "<Fun><Name>Randomizer</Name><Base>" + ui->randomizerBaseEdit->text() +  "</Base><Deviation>";
    stringbuffer = stringbuffer + ui->randomizerDeviationEdit->text() +  "</Deviation></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* RandomDensity Signal Functions */
void FunctionGenerator::randomDensityEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>RandomDensity</Name><Envelope>" + ui->randomDensityEnvEdit->text() + "</Envelope>";
    stringbuffer = stringbuffer + "<Low>" + ui->randomDensityStartTimeEdit->text() +  "</Low>";
    stringbuffer = stringbuffer + "<High>" + ui->randomDensityEndTimeEdit->text() +  "</High></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* Decay Signal Functions */
void FunctionGenerator::decayBaseFunButtonClicked(){
  QString initialText = ui->decayBaseEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->decayBaseEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::decayRateFunButtonClicked(){
  QString initialText = ui->decayRateEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->decayRateEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::decayIndexFunButtonClicked(){
  QString initialText = ui->decayIndexEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->decayIndexEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::decayEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>Decay</Name><Base>" + ui->decayBaseEdit->text() +  "</Base><Type>";
    if (ui->decayTypeExponential->isChecked()) { stringbuffer = stringbuffer + "EXPONENTIAL"; }
    else if (ui->decayTypeLinear->isChecked()) { stringbuffer = stringbuffer + "LINEAR"; }
    stringbuffer = stringbuffer + "</Type><Rate>" + ui->decayRateEdit->text() + "</Rate><Index>";
    stringbuffer = stringbuffer + ui->decayIndexEdit->text() + "</Index></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* Inverse Signal Functions */
void FunctionGenerator::inverseFunButtonClicked(){
  QString initialText = ui->numToInverseEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->numToInverseEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::inverseEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>Inverse</Name><Entry>" + ui->numToInverseEdit->text() + "</Entry></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* Markov Signal Functions */
void FunctionGenerator::markovEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>Markov</Name><Entry>" + ui->markovEdit->text() + "</Entry></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* LN Signal Functions */
void FunctionGenerator::LNFunButtonClicked(){
  QString initialText = ui->lnEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->lnEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::LNEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>LN</Name><Entry>" + ui->lnEdit->text() + "</Entry></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* Fibonacci Signal Functions */
void FunctionGenerator::FibonacciFunButtonClicked(){
  QString initialText = ui->fibEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->fibEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::FibonacciEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>Fibonacci</Name><Entry>" + ui->fibEdit->text() + "</Entry></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* Select Signal Functions */
void FunctionGenerator::selectIndexFunButtonClicked(){
  QString initialText = ui->selectIndexEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->selectIndexEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::selectTextChanged(){
    QString stringbuffer;
    stringbuffer =  "<Fun><Name>Select</Name><List>";
    
    QString nodesText = "";
    for (int i = 0; i < ui->selectScrollLayout->count() - 1; ++i) {
        QLayoutItem* item = ui->selectScrollLayout->itemAt(i);
        if (item && item->widget()) {
            Select* node = qobject_cast<Select*>(item->widget());
            if (node) { nodesText += node->getNodeText(); }
            if (i < ui->selectScrollLayout->count() - 2) { nodesText += ", "; }
        }
    }
    stringbuffer = stringbuffer + nodesText;

    stringbuffer =  stringbuffer + "</List><Index>" + ui->selectIndexEdit->text() + "</Index></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::addSelectNodeButtonClicked() {
  int index = ui->selectScrollLayout->count() - 1;
  Select* node = new Select(index, ui->selectScrollWindowNodes);

  connect(node, &Select::deleteRequested, this, &FunctionGenerator::removeSelectNodeButtonClicked);
  connect(node, &Select::nodeTextChanged, this, &FunctionGenerator::selectTextChanged);
  connect(node, &Select::editFocused, this, [this](QLineEdit* edit){ this->m_lastFocusedSelectEdit = edit; });

  ui->selectScrollLayout->insertWidget(index, node);
  ui->selectScrollWindowNodes->adjustSize();
  selectTextChanged();
}
void FunctionGenerator::removeSelectNodeButtonClicked(Select* node) {
    if (!node) return;
    int deletedIndex = node->getSelectIndex(); 

    if (m_lastFocusedSelectEdit && node->isAncestorOf(m_lastFocusedSelectEdit)) {
        m_lastFocusedSelectEdit = nullptr;
    }

    ui->selectScrollLayout->removeWidget(node);
    node->deleteLater();

    for (int i = deletedIndex; i < ui->selectScrollLayout->count() - 1; ++i) {
        QLayoutItem* item = ui->selectScrollLayout->itemAt(i);
        if (item && item->widget()) {
            Select* remainingNode = qobject_cast<Select*>(item->widget());
            if (remainingNode) {
                remainingNode->setSelectIndex(i); 
            }
        }
    }
    ui->selectScrollWindowNodes->adjustSize();
    selectTextChanged(); 
}
void FunctionGenerator::selectFunButtonClicked() {
  if (!m_lastFocusedSelectEdit) { return; }
  QString initialText = m_lastFocusedSelectEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnMakeListFun, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        m_lastFocusedSelectEdit->setText(result);
    }
  }
  delete generator;
}

/* Stochos Signal Functions */
void FunctionGenerator::stochosTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>Stochos</Name><Method>";
    if (ui->stochosMethodRange->isChecked()) { stringbuffer = stringbuffer + "RANGE_DISTRIB"; }
    else if (ui->stochosMethodFunction->isChecked()) { stringbuffer = stringbuffer + "FUNCTIONS"; }
    stringbuffer = stringbuffer + "</Method><Envelopes>";
    
    QString nodesText = "";
    for (int i = 0; i < ui->stochosScrollLayout->count() - 1; ++i) {
        QLayoutItem* item = ui->stochosScrollLayout->itemAt(i);
        if (item && item->widget()) {
            Stochos* node = qobject_cast<Stochos*>(item->widget());
            if (node) { nodesText += node->getNodeText(); }
        }
    }
    stringbuffer = stringbuffer + nodesText;

    stringbuffer = stringbuffer + "</Envelopes><Offset>" + ui->stochosOffsetEdit->text() + "</Offset></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::addStochosNodeButtonClicked() {
  int method = ui->stochosMethodRange->isChecked() ? 0 : 1;
  int index = ui->stochosScrollLayout->count() - 1;
  Stochos* node = new Stochos(method, index, ui->stochosScrollWindowNodes);

  connect(node, &Stochos::deleteRequested, this, &FunctionGenerator::removeStochosNodeButtonClicked);
  connect(node, &Stochos::nodeTextChanged, this, &FunctionGenerator::stochosTextChanged);
  connect(node, &Stochos::editFocused, this, [this](QLineEdit* edit){ this->m_lastFocusedStochosEdit = edit; });

  ui->stochosScrollLayout->insertWidget(index, node);
  ui->stochosScrollWindowNodes->adjustSize();
  stochosTextChanged();
}
void FunctionGenerator::removeStochosNodeButtonClicked(Stochos* node) {
    if (!node) return;
    int deletedIndex = node->getStochosIndex(); 

    if (m_lastFocusedStochosEdit && node->isAncestorOf(m_lastFocusedStochosEdit)) {
        m_lastFocusedStochosEdit = nullptr;
    }

    ui->stochosScrollLayout->removeWidget(node);
    node->deleteLater();

    for (int i = deletedIndex; i < ui->stochosScrollLayout->count() - 1; ++i) {
        QLayoutItem* item = ui->stochosScrollLayout->itemAt(i);
        if (item && item->widget()) {
            Stochos* remainingNode = qobject_cast<Stochos*>(item->widget());
            if (remainingNode) {
                remainingNode->setStochosIndex(i); 
            }
        }
    }
    ui->stochosScrollWindowNodes->adjustSize();
    stochosTextChanged(); 
}
void FunctionGenerator::clearStochosNodes() {
  if (!ui->stochosScrollLayout) return;
  while (ui->stochosScrollLayout->count() > 1) {
    QLayoutItem* item = ui->stochosScrollLayout->takeAt(0);
    if (item) {
      if (QWidget* widget = item->widget()) { widget->deleteLater(); }
      delete item;
    }
  }
  stochosTextChanged();
}
void FunctionGenerator::stochosFunButtonClicked() {
  if (!m_lastFocusedStochosEdit) { return; }
  QString initialText = m_lastFocusedStochosEdit->text();
  FunctionReturnType type = functionReturnENV;
  if (m_lastFocusedStochosEdit == ui->stochosOffsetEdit) { type = functionReturnInt; }
  FunctionGenerator* generator = new FunctionGenerator(this, type, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        m_lastFocusedStochosEdit->setText(result);
    }
  }
  delete generator;
}

/* ValuePick Signal Functions */
void FunctionGenerator::valuePickAbsRangeFunButtonClicked(){
  QString initialText = ui->valuePickAbsRangeEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->valuePickAbsRangeEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::valuePickLowFunButtonClicked(){
  QString initialText = ui->valuePickLowEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnENV, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->valuePickLowEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::valuePickHighFunButtonClicked(){
  QString initialText = ui->valuePickHighEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnENV, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->valuePickHighEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::valuePickDistFunButtonClicked(){
  QString initialText = ui->valuePickDistEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnENV, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->valuePickDistEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::valuePickTextChanged(){
    QString stringbuffer;
    stringbuffer = stringbuffer + "<Fun><Name>ValuePick</Name><Range>"+ ui->valuePickAbsRangeEdit->text() + "</Range><Low>";
    stringbuffer = stringbuffer + ui->valuePickLowEdit->text()+ "</Low><High>";
    stringbuffer = stringbuffer + ui->valuePickHighEdit->text()+ "</High><Dist>";
    stringbuffer = stringbuffer + ui->valuePickDistEdit->text()+ "</Dist><Method>";
    if (ui->valuePickElementsMeaningful->isChecked()) { stringbuffer = stringbuffer + "MEANINGFUL"; }
    else if (ui->valuePickElementsMods->isChecked()) { stringbuffer = stringbuffer + "MODS"; }
    else if (ui->valuePickElementsFake->isChecked()) { stringbuffer = stringbuffer + "FAKE"; }
    else if (ui->valuePickElementsFib->isChecked()) { stringbuffer = stringbuffer + "FIBONACCI"; }
    stringbuffer = stringbuffer + "</Method><Elements>" + ui->valuePickValuesElementsEdit->text() + "</Elements><WeightMethod>";
    if (ui->valuePickWeightsPeriodic->isChecked()) { stringbuffer = stringbuffer + "PERIODIC"; }
    else if (ui->valuePickWeightsHierarchic->isChecked()) { stringbuffer = stringbuffer + "HIERARCHIC"; }
    else if (ui->valuePickWeightsInclude->isChecked()) { stringbuffer = stringbuffer + "INCLUDE"; }
    stringbuffer = stringbuffer + "</WeightMethod><Weight>" + ui->valuePickValuesWeightsEdit->text() + "</Weight><Type>";
    if (ui->valuePickTypeVariable->isChecked()) { stringbuffer = stringbuffer + "VARIABLE"; }
    else if (ui->valuePickTypeConstant->isChecked()) { stringbuffer = stringbuffer + "CONSTANT"; }
    stringbuffer = stringbuffer + "</Type><Offsets>" + ui->valuePickOffsetEdit->text() + "</Offsets></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* ChooseL Signal Functions */
void FunctionGenerator::chooseLFunButtonClicked(){
  QString initialText = ui->chooseLEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnSIV, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->chooseLEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::chooseLEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ChooseL</Name><Entry>" + ui->chooseLEdit->text() +  "</Entry></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* GetPattern Signal Functions */
void FunctionGenerator::getPatternOffsetFunButtonClicked(){
  QString initialText = ui->getPatternOriginEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->getPatternOriginEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::getPatternFunButtonClicked(){
  QString initialText = ui->getPatternChooseEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnPAT, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->getPatternChooseEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::getPatternEntryChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>GetPattern</Name><Method>";
    if (ui->getPatternMethodInOrder->isChecked()) { stringbuffer = stringbuffer + "IN_ORDER"; }
    else if (ui->getPatternMethodOther->isChecked()) { stringbuffer = stringbuffer + "OTHER"; }
    else if (ui->getPatternMethodTypeClusters->isChecked()) { stringbuffer = stringbuffer + "TYPE_CLUSTERS"; }
    else if (ui->getPatternMethodTimeDepend->isChecked()) { stringbuffer = stringbuffer + "TIME_DEPEND"; }
    else if (ui->getPatternMethodProbability->isChecked()) { stringbuffer = stringbuffer + "PROBABILITY"; }
    stringbuffer = stringbuffer + "</Method><Offset>" + ui->getPatternOriginEdit->text() + "</Offset><Pattern>";
    stringbuffer = stringbuffer + ui->getPatternChooseEdit->text() + "</Pattern></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* MakeList Signal Functions */
void FunctionGenerator::makeListFunctionFunButtonClicked(){
  QString initialText = ui->makeListFunctionEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnMakeListFun, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeListFunctionEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeListSizeFunButtonClicked(){
  QString initialText = ui->makeListSizeEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnMakeListFun, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeListSizeEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeListTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>MakeList</Name><Func>" + ui->makeListFunctionEdit->text() + "</Func><Size>";
    stringbuffer = stringbuffer + ui->makeListSizeEdit->text() + "</Size></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* EnvLib Signal Functions */
void FunctionGenerator::envLibEnvelopeFunButtonClicked(){
  QString initialText = ui->envLibNumEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->envLibNumEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::envLibScalingFactorFunButtonClicked(){
  QString initialText = ui->envLibScalingEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->envLibScalingEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::envLibTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>EnvLib</Name><Env>" + ui->envLibNumEdit->text() + "</Env><Scale>";
    stringbuffer = stringbuffer + ui->envLibScalingEdit->text() + "</Scale></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* MakeEnvelope Signal Functions */
void FunctionGenerator::makeEnvelopeScalingFactorFunButtonClicked(){
  QString initialText = ui->makeEnvelopeScalingEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeEnvelopeScalingEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeEnvelopeXValueFunButtonClicked(){
  QString initialText = ui->makeEnvelopeXEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeEnvelopeXEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeEnvelopeYValueFunButtonClicked(){
  QString initialText = ui->makeEnvelopeYEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeEnvelopeYEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeEnvelopeTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>MakeEnvelope</Name><Xs>";
    // TO DO: MakeEnvelopeSubAlignment
    stringbuffer = stringbuffer + "<X>" + ui->makeEnvelopeXEdit->text() + "</X></Xs><Ys>";
    stringbuffer = stringbuffer + "<Y>" + ui->makeEnvelopeYEdit->text() + "</Y></Ys><Types>";
    stringbuffer = stringbuffer + "</Types><Pros>";
    stringbuffer = stringbuffer+ "</Pros><Scale>" + ui->makeEnvelopeScalingEdit->text() + "</Scale></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* MakePattern Signal Functions */
void FunctionGenerator::makePatternIntervalsFunButtonClicked(){
  QString initialText = ui->makePatternIntervalsEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnList, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makePatternIntervalsEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makePatternTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>MakePattern</Name><List>" + ui->makePatternIntervalsEdit->text() + "</List></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* ExpandPattern Signal Functions */
void FunctionGenerator::expandPatternModuloFunButtonClicked(){
  QString initialText = ui->expandPatternModuloEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->expandPatternModuloEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::expandPatternLowFunButtonClicked(){
  QString initialText = ui->expandPatternLowEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->expandPatternLowEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::expandPatternHighFunButtonClicked(){
  QString initialText = ui->expandPatternHighEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnInt, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->expandPatternHighEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::expandPatternPatternFunButtonClicked(){
  QString initialText = ui->expandPatternPatternEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnPAT, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->expandPatternPatternEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::expandPatternTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ExpandPattern</Name><Method>";
    if (ui->expandPatternMethodEquivalence->isChecked()) { stringbuffer = stringbuffer + "EQUIVALENCE"; }
    else if (ui->expandPatternMethodSymmetries->isChecked()) { stringbuffer = stringbuffer + "SYMMETRIES"; }
    else if (ui->expandPatternMethodDistort->isChecked()) { stringbuffer = stringbuffer + "DISTORT"; }
    stringbuffer = stringbuffer + "</Method><Modulo>" + ui->expandPatternModuloEdit->text() +  "</Modulo><Low>";
    stringbuffer = stringbuffer + ui->expandPatternLowEdit->text() + "</Low><High>";
    stringbuffer = stringbuffer + ui->expandPatternHighEdit->text() + "</High><Pattern>";
    stringbuffer = stringbuffer + ui->expandPatternPatternEdit->text() + "</Pattern></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* MakeFilter Signal Functions */
void FunctionGenerator::makeFilterFrequencyFunButtonClicked(){
  QString initialText = ui->makeFilterFrequencyEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeFilterFrequencyEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeFilterBandWidthFunButtonClicked(){
  QString initialText = ui->makeFilterBWEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeFilterBWEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeFilterDBGainFunButtonClicked(){
  QString initialText = ui->makeFilterDBEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeFilterDBEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeFilterTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>MakeFilter</Name><Type>";
    if (ui->makeFilterLPF->isChecked()) {
        stringbuffer = stringbuffer + "LPF";
        ui->makeFilterDBEdit->setEnabled(false);
        ui->makeFilterDBInsertFn->setEnabled(false);
    }
    if (ui->makeFilterHPF->isChecked()) {
        stringbuffer = stringbuffer + "HPF";
        ui->makeFilterDBEdit->setEnabled(false);
        ui->makeFilterDBInsertFn->setEnabled(false);
    }
    if (ui->makeFilterBPF->isChecked()) {
        stringbuffer = stringbuffer + "BPF";
        ui->makeFilterDBEdit->setEnabled(false);
        ui->makeFilterDBInsertFn->setEnabled(false);
    }
    if (ui->makeFilterHSF->isChecked()) {
        stringbuffer = stringbuffer + "HSF";
        ui->makeFilterDBEdit->setEnabled(true);
        ui->makeFilterDBInsertFn->setEnabled(true);
    }
    if (ui->makeFilterLSF->isChecked()) {
        stringbuffer = stringbuffer + "LSF";
        ui->makeFilterDBEdit->setEnabled(true);
        ui->makeFilterDBInsertFn->setEnabled(true);
    }
    if (ui->makeFilterNF->isChecked()) {
        stringbuffer = stringbuffer + "NF";
        ui->makeFilterDBEdit->setEnabled(false);
        ui->makeFilterDBInsertFn->setEnabled(false);
    }
    if (ui->makeFilterPBEQ->isChecked()) {
        stringbuffer = stringbuffer + "PBEQF";
        ui->makeFilterDBEdit->setEnabled(true);
        ui->makeFilterDBInsertFn->setEnabled(true);
    }
    stringbuffer = stringbuffer +"</Type><Frequency>" + ui->makeFilterFrequencyEdit->text()+ "</Frequency>";
    stringbuffer = stringbuffer +"<BandWidth>" + ui->makeFilterBWEdit->text()+ "</BandWidth>";
    stringbuffer = stringbuffer +"<dBGain>" + ui->makeFilterDBEdit->text()+ "</dBGain></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* Read File Signal Functions */
void FunctionGenerator::readENVFileTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ReadENVFile</Name><File>" + ui->readEnvFileEdit->text() +  "</File></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::readPATFileTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ReadPATFile</Name><File>" + ui->readPatFileEdit->text() +  "</File></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::readFILFileTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ReadFILFile</Name><File>" + ui->readFilFileEdit->text() +  "</File></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::readSIVFileTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ReadSIVFile</Name><File>" + ui->readSivFileEdit->text() +  "</File></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::readSPAFileTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ReadSPAFile</Name><File>" + ui->readSpaFileEdit->text() +  "</File></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::readREVFileTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>ReadREVFile</Name><File>" + ui->readRevFileEdit->text() +  "</File></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* REV Signal Functions */
void FunctionGenerator::REVTextChanged(){
    QString stringbuffer;
    int method;
    int apply;
    switch (REVMethodFlag) {
        case 0: stringbuffer = "<Fun><Name>REV_Simple</Name><Apply>"; break;
        case 1: stringbuffer = "<Fun><Name>REV_Medium</Name><Apply>"; break;
        case 2: stringbuffer = "<Fun><Name>REV_Advanced</Name><Apply>"; break;
    }
    if (ui->revApplySound->isChecked()) { stringbuffer += "SOUND"; }
    else { stringbuffer += "PARTIAL"; }
    stringbuffer += "</Apply>";

    switch (REVMethodFlag) {
        case 0:
            stringbuffer += "<Sizes>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlRoom(); }
            stringbuffer += "</Sizes>";
            break;
        case 1:
            stringbuffer += "<Percents>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlReverb(); }
            stringbuffer += "</Percents>";
            stringbuffer += "<Spreads>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlHillow(); }
            stringbuffer += "</Spreads>";
            stringbuffer += "<AllPasses>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlAllGain(); }
            stringbuffer += "</AllPasses>";
            stringbuffer += "<Delays>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlDelay(); }
            stringbuffer += "</Delays>";
            break;
        case 2:
            stringbuffer += "<Percents>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlReverb(); }
            stringbuffer += "</Percents>";
            stringbuffer += "<CombGainLists>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlCombGain(); }
            stringbuffer += "</CombGainLists>";
            stringbuffer += "<LPGainLists>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlLPGain(); }
            stringbuffer += "</LPGainLists>";
            stringbuffer += "<AllPasses>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlAllGain(); }
            stringbuffer += "</AllPasses>";
            stringbuffer += "<Delays>";
            for (REVChannel* chan : m_revChannels) { stringbuffer += chan->getXmlDelay(); }
            stringbuffer += "</Delays>";
            break;
    }   
    stringbuffer += "</Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
REVChannel* FunctionGenerator::REVInsertChannel(REVChannel* prevCha) {
    int insertIdx = prevCha ? m_revChannels.indexOf(prevCha) + 1 : 0;

    REVChannel* newCha = new REVChannel(m_revChannels.size() + 1, REVMethodFlag, ui->revScrollWindowContent);
    m_revChannels.insert(insertIdx, newCha);
    ui->revScrollWindowLayout->insertWidget(insertIdx, newCha);
    connect(newCha, &REVChannel::textChanged, this, &FunctionGenerator::REVTextChanged);
    connect(newCha, &REVChannel::insertChannelRequested, this, &FunctionGenerator::REVInsertChannel);
    connect(newCha, &REVChannel::removeChannelRequested, this, &FunctionGenerator::REVRemoveChannel);

    updateRevChaLabels();
    REVTextChanged();
    return newCha;
}
void FunctionGenerator::REVRemoveChannel(REVChannel* currCha) {
    if (m_revChannels.size() <= 1) return;

    int idx = m_revChannels.indexOf(currCha);
    if (idx != -1) {
        m_revChannels.takeAt(idx);
        currCha->deleteLater();
    }

    updateRevChaLabels();
    REVTextChanged();
}
void FunctionGenerator::handleRevApplyMethodChanged() {
    if (!m_revChannels.isEmpty()) clearRevChannels();
    REVChannel* first = REVInsertChannel(nullptr);

    if (ui->revApplySound->isChecked()) {
        first->setTitle("Sound");
        first->hideButtons();
    } 
}
void FunctionGenerator::updateRevChaLabels() {
    for (int i = 0; i < m_revChannels.size(); ++i) {
        if (!ui->revApplySound->isChecked()) {
            m_revChannels[i]->setTitle(QString("Partial %1:").arg(i + 1));
        }
    }
}
void FunctionGenerator::clearRevChannels() {
    for (REVChannel* chan : m_revChannels) {
        ui->revScrollWindowLayout->removeWidget(chan);
        delete chan;
    }
    m_revChannels.clear();
    REVTextChanged();
}


/* MakeSieve Signal Functions */
void FunctionGenerator::makeSieveLowFunButtonClicked(){
  QString initialText = ui->makeSieveLowBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnENV, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeSieveLowBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeSieveHighFunButtonClicked(){
  QString initialText = ui->makeSieveHighBoundEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnENV, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->makeSieveHighBoundEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::makeSieveTextChanged(){
    QString stringbuffer;
    stringbuffer = "<Fun><Name>MakeSieve</Name><Low>"+ ui->makeSieveLowBoundEdit->text() + "</Low><High>";
    stringbuffer = stringbuffer + ui->makeSieveHighBoundEdit->text() + "</High><Method>";
    if (ui->makeSieveElementsMeaningful->isChecked()) { stringbuffer = stringbuffer + "MEANINGFUL"; }
    else if (ui->makeSieveElementsMods->isChecked()) { stringbuffer = stringbuffer + "MODS"; }
    else if (ui->makeSieveElementsFake->isChecked()) { stringbuffer = stringbuffer + "FAKE"; }
    else if (ui->makeSieveElementsFib->isChecked()) { stringbuffer = stringbuffer + "FIBONACCI"; }
    stringbuffer = stringbuffer + "</Method><Elements>" + ui->makeSieveElementsValuesEdit->text() + "</Elements><WeightMethod>";
    if (ui->makeSieveWeightsPeriodic->isChecked()) { stringbuffer = stringbuffer + "PERIODIC"; }
    else if (ui->makeSieveWeightsHierarchic->isChecked()) { stringbuffer = stringbuffer + "HIERARCHIC"; }
    else if (ui->makeSieveWeightsInclude->isChecked()) { stringbuffer = stringbuffer + "INCLUDE"; }
    stringbuffer = stringbuffer + "</WeightMethod><Weight>" + ui->makeSieveWeightsValuesEdit->text() + "</Weight><Offset>";
    stringbuffer = stringbuffer + ui->makeSieveOffsetEdit->text() + "</Offset></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}

/* SPA Signal Functions */
void FunctionGenerator::SPATextChanged() {
    QString stringbuffer;
    int method;
    int apply;
    stringbuffer = "<Fun><Name>SPA</Name><Method>";
    if (ui->spaStereo->isChecked()) {
        stringbuffer = stringbuffer + "STEREO";
        method = 0;
    } else if (ui->spaMultiPan->isChecked()) {
        stringbuffer = stringbuffer + "MULTI_PAN";
        method = 1;
    } else if (ui->spaPolar->isChecked()) {
        stringbuffer = stringbuffer + "POLAR";
        method = 2;
    }
    stringbuffer = stringbuffer + "</Method><Apply>";
    if (ui->spaApplySound->isChecked()) {
        stringbuffer = stringbuffer + "SOUND";
        apply = 0;
    } else if (ui->spaApplyPartial->isChecked()) {
        stringbuffer = stringbuffer + "PARTIAL";
        apply = 1;
    }
    stringbuffer = stringbuffer + "</Apply><Channels>";

    for (SPAChannel* chan : m_spaChannels) {
        stringbuffer += "<Partials>";
        stringbuffer += chan->getXml();
        stringbuffer += "</Partials>";
    }

    stringbuffer = stringbuffer + "</Channels></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}
void FunctionGenerator::handleSpaPartialSyncInsert(int index) {
    for (auto* chan : m_spaChannels) {
        chan->addRow(index + 1, true);
    }
    SPATextChanged();
}
void FunctionGenerator::handleSpaPartialSyncRemove(int index) {
    for (auto* chan : m_spaChannels) {
        chan->removeRow(index);
    }
    SPATextChanged();
}
SPAChannel* FunctionGenerator::SPAInsertChannel(SPAChannel* prevCha) {
    int insertIdx = prevCha ? m_spaChannels.indexOf(prevCha) + 1 : 0;
    bool isPartialMode = ui->spaApplyPartial->isChecked();

    SPAChannel* newCha = new SPAChannel(m_spaChannels.size() + 1, isPartialMode, ui->spaScrollWindowContent);
    
    m_spaChannels.insert(insertIdx, newCha);
    ui->spaScrollWindowLayout->insertWidget(insertIdx, newCha);

    int targetRowCount = m_spaChannels.isEmpty() ? 1 : m_spaChannels.first()->rowCount();
    for (int i = 0; i < targetRowCount; ++i) {
        newCha->addRow(i, true); // Silent add to match others
    }
    if (newCha->rowCount() == 0) { newCha->addRow(0, true); }

    connect(newCha, &SPAChannel::textChanged, this, &FunctionGenerator::SPATextChanged);
    connect(newCha, &SPAChannel::insertChannelRequested, this, &FunctionGenerator::SPAInsertChannel);
    connect(newCha, &SPAChannel::removeChannelRequested, this, &FunctionGenerator::SPARemoveChannel);
    connect(newCha, &SPAChannel::rowInsertRequested, this, &FunctionGenerator::handleSpaPartialSyncInsert);
    connect(newCha, &SPAChannel::rowRemoveRequested, this, &FunctionGenerator::handleSpaPartialSyncRemove);

    updateChaLabels();
    SPATextChanged();
    return newCha;
}
void FunctionGenerator::SPARemoveChannel(SPAChannel* currCha) {
    if (m_spaChannels.size() <= 1) return;

    int idx = m_spaChannels.indexOf(currCha);
    if (idx != -1) {
        m_spaChannels.takeAt(idx);
        currCha->deleteLater();
    }

    updateChaLabels();
    SPATextChanged();
}
void FunctionGenerator::handleSpaApplyMethodChanged() {
    clearSpaChannels();
    
    if (ui->spaStereo->isChecked()) {
        SPAChannel* stereo = SPAInsertChannel(nullptr);
        stereo->hideTitle();
        stereo->hideButtons();
    } 
    else if (ui->spaMultiPan->isChecked()) {
        SPAInsertChannel(nullptr);
    } 
    else if (ui->spaPolar->isChecked()) {
        SPAChannel* theta = SPAInsertChannel(nullptr);
        theta->setTitle("Theta");
        theta->hideButtons();
        
        SPAChannel* rad = SPAInsertChannel(theta);
        rad->setTitle("Radius");
        rad->hideButtons();
    }
}
void FunctionGenerator::updateChaLabels() {
    for (int i = 0; i < m_spaChannels.size(); ++i) {
        if (!ui->spaPolar->isChecked()) {
            m_spaChannels[i]->setTitle(QString("Channel %1:").arg(i + 1));
        }
    }
}
void FunctionGenerator::clearSpaChannels() {
    for (SPAChannel* chan : m_spaChannels) {
        chan->deleteLater();
    }
    m_spaChannels.clear();
    SPATextChanged();
}

/* SpectrumGen Signal Functions */
void FunctionGenerator::Spectrum_GenEnvelopeFunButtonClicked(){
  QString initialText = ui->spectrumGenEnvelopeEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnENV, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->spectrumGenEnvelopeEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::Spectrum_GenDistanceFunButtonClicked(){
  QString initialText = ui->spectrumGenDistanceEdit->text();
  FunctionGenerator* generator = new FunctionGenerator(this, functionReturnFloat, initialText);
  if (generator->exec() == QDialog::Accepted) {
    QString result = generator->getResultString();
    if (!result.isEmpty()) {
        ui->spectrumGenDistanceEdit->setText(result);
    }
  }
  delete generator;
}
void FunctionGenerator::Spectrum_GenTextChanged() {
    QString stringbuffer;
    stringbuffer =  "<Fun><Name>Spectrum_Gen</Name><Envelope>" + ui->spectrumGenEnvelopeEdit->text() +  "</Envelope><Distance>";
    stringbuffer =  stringbuffer + ui->spectrumGenDistanceEdit->text() + "</Distance></Fun>";
    ui->resultTextEdit->setText(stringbuffer);
}




