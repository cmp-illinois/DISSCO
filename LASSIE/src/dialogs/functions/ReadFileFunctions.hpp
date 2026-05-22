#ifndef READFILEFUNCTIONS_HPP
#define READFILEFUNCTIONS_HPP

// The Read*File family: each takes a single file-path text entry, no
// fn button, and serializes as <Fun><Name>ReadXFile</Name><File>P</File></Fun>.
// Defined together because the six classes are byte-for-byte the same
// shape; only the enum id, xml/display name, label, and return type differ.

#include "MultiEntryFunction.hpp"

class ReadENVFileFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit ReadENVFileFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({{ tr("ENV File Name:"), "File",
              FunctionReturnType::functionReturnENV, {}, /*fnVisible=*/false }}, parent) {}
    CMODFunction id() const override { return CMODFunction::functionReadENVFile; }
    QString xmlName() const override { return QStringLiteral("ReadENVFile"); }
    QString displayName() const override { return QStringLiteral("ReadENVFile"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnENV };
    }
};

class ReadSIVFileFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit ReadSIVFileFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({{ tr("SIV File Name:"), "File",
              FunctionReturnType::functionReturnSIV, {}, /*fnVisible=*/false }}, parent) {}
    CMODFunction id() const override { return CMODFunction::functionReadSIVFile; }
    QString xmlName() const override { return QStringLiteral("ReadSIVFile"); }
    QString displayName() const override { return QStringLiteral("ReadSIVFile"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnSIV };
    }
};

class ReadPATFileFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit ReadPATFileFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({{ tr("PAT File Name:"), "File",
              FunctionReturnType::functionReturnPAT, {}, /*fnVisible=*/false }}, parent) {}
    CMODFunction id() const override { return CMODFunction::functionReadPATFile; }
    QString xmlName() const override { return QStringLiteral("ReadPATFile"); }
    QString displayName() const override { return QStringLiteral("ReadPATFile"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnPAT };
    }
};

class ReadSPAFileFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit ReadSPAFileFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({{ tr("SPA File Name:"), "File",
              FunctionReturnType::functionReturnSPA, {}, /*fnVisible=*/false }}, parent) {}
    CMODFunction id() const override { return CMODFunction::functionReadSPAFile; }
    QString xmlName() const override { return QStringLiteral("ReadSPAFile"); }
    QString displayName() const override { return QStringLiteral("ReadSPAFile"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnSPA };
    }
};

class ReadREVFileFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit ReadREVFileFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({{ tr("REV File Name:"), "File",
              FunctionReturnType::functionReturnREV, {}, /*fnVisible=*/false }}, parent) {}
    CMODFunction id() const override { return CMODFunction::functionReadREVFile; }
    QString xmlName() const override { return QStringLiteral("ReadREVFile"); }
    QString displayName() const override { return QStringLiteral("ReadREVFile"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnREV };
    }
};

class ReadFILFileFunction : public MultiEntryFunction {
    Q_OBJECT
public:
    explicit ReadFILFileFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({{ tr("FIL File Name:"), "File",
              FunctionReturnType::functionReturnFIL, {}, /*fnVisible=*/false }}, parent) {}
    CMODFunction id() const override { return CMODFunction::functionReadFILFile; }
    QString xmlName() const override { return QStringLiteral("ReadFILFile"); }
    QString displayName() const override { return QStringLiteral("ReadFILFile"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnFIL };
    }
};

#endif // READFILEFUNCTIONS_HPP
