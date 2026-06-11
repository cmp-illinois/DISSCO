#ifndef SPECTRUMGENFUNCTION_HPP
#define SPECTRUMGENFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class SpectrumGenFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit SpectrumGenFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Envelope:"), "Envelope",
                FunctionReturnType::functionReturnENV, {} },
              { tr("Distance:"), "Distance",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionSpectrum_Gen; }
    QString xmlName() const override { return QStringLiteral("Spectrum_Gen"); }
    QString displayName() const override { return QStringLiteral("Generate_Spectrum"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnSPE };
    }
};

#endif // SPECTRUMGENFUNCTION_HPP
