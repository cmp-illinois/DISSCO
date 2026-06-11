#ifndef REVFUNCTIONS_HPP
#define REVFUNCTIONS_HPP

#include "RevFunctionBase.hpp"

class RevSimpleFunction : public RevFunctionBase {
    Q_OBJECT
public:
    explicit RevSimpleFunction(QWidget* parent = nullptr)
        : RevFunctionBase(/*mode=*/0, parent) {}
    CMODFunction id() const override { return CMODFunction::functionREV_Simple; }
    QString xmlName() const override { return QStringLiteral("REV_Simple"); }
    QString displayName() const override { return QStringLiteral("REV_Simple"); }
};

class RevMediumFunction : public RevFunctionBase {
    Q_OBJECT
public:
    explicit RevMediumFunction(QWidget* parent = nullptr)
        : RevFunctionBase(/*mode=*/1, parent) {}
    CMODFunction id() const override { return CMODFunction::functionREV_Medium; }
    QString xmlName() const override { return QStringLiteral("REV_Medium"); }
    QString displayName() const override { return QStringLiteral("REV_Medium"); }
};

class RevAdvancedFunction : public RevFunctionBase {
    Q_OBJECT
public:
    explicit RevAdvancedFunction(QWidget* parent = nullptr)
        : RevFunctionBase(/*mode=*/2, parent) {}
    CMODFunction id() const override { return CMODFunction::functionREV_Advanced; }
    QString xmlName() const override { return QStringLiteral("REV_Advanced"); }
    QString displayName() const override { return QStringLiteral("REV_Advanced"); }
};

#endif // REVFUNCTIONS_HPP
