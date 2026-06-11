#ifndef FUNCTIONGENERATOR_HPP
#define FUNCTIONGENERATOR_HPP

#include "../lassie.hpp"

#include <QDialog>
#include <QMap>
#include <QString>

class FunctionWidget;

namespace Ui {
class FunctionGenerator;
}

/**
 * Dialog that hosts one FunctionWidget at a time, selected via a combo
 * box filtered by the requested return type. The dialog is now just a
 * thin shell -- it pulls the available functions from FunctionRegistry,
 * instantiates them on demand, and mirrors each widget's xmlChanged
 * signal into the result text edit. All per-function UI and slot logic
 * lives in the FunctionWidget subclasses under dialogs/functions/impl/.
 */
class FunctionGenerator : public QDialog
{
    Q_OBJECT

public:
    explicit FunctionGenerator(QWidget* parent = nullptr,
                               FunctionReturnType returnType = FunctionReturnType::functionReturnFloat,
                               QString originalString = QString());
    ~FunctionGenerator() override;

    QString getResultString();

private slots:
    void handleFunctionChanged(int index);

private:
    void setupUi();

    // Lazily build (and cache) the registered FunctionWidget for `id`,
    // appending it to functionStackedWidget on first use. Returns nullptr
    // if no subclass is registered for that id.
    FunctionWidget* ensureRegisteredWidget(CMODFunction id);

    // Set the current stacked-widget page to Expanding, the rest to
    // Ignored, then resize the dialog to fit the new page's sizeHint.
    void resizeToFitCurrentPage();

    Ui::FunctionGenerator* ui;
    FunctionReturnType m_returnType;
    QString m_originalString;
    QString m_result;

    QMap<CMODFunction, FunctionWidget*> m_registeredWidgets;
    QMap<CMODFunction, int>             m_registeredPageIndex;
};

#endif // FUNCTIONGENERATOR_HPP
