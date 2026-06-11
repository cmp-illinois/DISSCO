#ifndef STATICCONSTANTFUNCTIONS_HPP
#define STATICCONSTANTFUNCTIONS_HPP

// The function_static* family: no input UI, just emit <Fun><Name>X</Name></Fun>
// when selected. MultiEntryFunction with an empty spec list handles both the
// (trivial) widget and the XML output.

#include "../MultiEntryFunction.hpp"

// Each class is spelled out rather than macro-generated because MOC
// scans for Q_OBJECT pre-preprocessor.
class CurrentTypeFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit CurrentTypeFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({}, parent) {}
    CMODFunction id() const override { return CMODFunction::function_staticCURRENT_TYPE; }
    QString xmlName() const override { return QStringLiteral("CURRENT_TYPE"); }
    QString displayName() const override { return QStringLiteral("CURRENT_TYPE"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnInt,
                 FunctionReturnType::functionReturnFloat,
                 FunctionReturnType::functionReturnMakeListFun };
    }
};

class CurrentChildNumFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit CurrentChildNumFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({}, parent) {}
    CMODFunction id() const override { return CMODFunction::function_staticCURRENT_CHILD_NUM; }
    QString xmlName() const override { return QStringLiteral("CURRENT_CHILD_NUM"); }
    QString displayName() const override { return QStringLiteral("CURRENT_CHILD_NUM"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnInt,
                 FunctionReturnType::functionReturnFloat,
                 FunctionReturnType::functionReturnMakeListFun };
    }
};

class CurrentPartialNumFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit CurrentPartialNumFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({}, parent) {}
    CMODFunction id() const override { return CMODFunction::function_staticCURRENT_PARTIAL_NUM; }
    QString xmlName() const override { return QStringLiteral("CURRENT_PARTIAL_NUM"); }
    QString displayName() const override { return QStringLiteral("CURRENT_PARTIAL_NUM"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnInt,
                 FunctionReturnType::functionReturnFloat,
                 FunctionReturnType::functionReturnMakeListFun };
    }
};

class CurrentSegmentFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit CurrentSegmentFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({}, parent) {}
    CMODFunction id() const override { return CMODFunction::function_staticCURRENT_SEGMENT; }
    QString xmlName() const override { return QStringLiteral("CURRENT_SEGMENT"); }
    QString displayName() const override { return QStringLiteral("CURRENT_SEGMENT"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnInt,
                 FunctionReturnType::functionReturnFloat,
                 FunctionReturnType::functionReturnMakeListFun };
    }
};

class AvailableEduFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit AvailableEduFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({}, parent) {}
    CMODFunction id() const override { return CMODFunction::function_staticAVAILABLE_EDU; }
    QString xmlName() const override { return QStringLiteral("AVAILABLE_EDU"); }
    QString displayName() const override { return QStringLiteral("AVAILABLE_EDU"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnInt,
                 FunctionReturnType::functionReturnFloat,
                 FunctionReturnType::functionReturnMakeListFun };
    }
};

class PreviousChildDurationFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit PreviousChildDurationFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({}, parent) {}
    CMODFunction id() const override { return CMODFunction::function_staticPREVIOUS_CHILD_DURATION; }
    QString xmlName() const override { return QStringLiteral("PREVIOUS_CHILD_DURATION"); }
    QString displayName() const override { return QStringLiteral("PREVIOUS_CHILD_DURATION"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnInt,
                 FunctionReturnType::functionReturnFloat,
                 FunctionReturnType::functionReturnMakeListFun };
    }
};

class CurrentLayerFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit CurrentLayerFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({}, parent) {}
    CMODFunction id() const override { return CMODFunction::function_staticCURRENT_LAYER; }
    QString xmlName() const override { return QStringLiteral("CURRENT_LAYER"); }
    QString displayName() const override { return QStringLiteral("CURRENT_LAYER"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnInt,
                 FunctionReturnType::functionReturnFloat,
                 FunctionReturnType::functionReturnMakeListFun };
    }
};

#endif // STATICCONSTANTFUNCTIONS_HPP
