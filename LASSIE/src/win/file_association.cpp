#include "file_association.hpp"

#if defined(_WIN32) && defined(LASSIE_CLAIM_DISSCO)

#include <windows.h>
#include <shlobj.h>

#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QString>

namespace {

// Registry layout we write under HKCU\Software\Classes:
//
//   .dissco                         (Default)  = "DISSCO.Project"
//   DISSCO.Project                  (Default)  = "DISSCO Project"
//                                   FriendlyTypeName = "DISSCO Project"
//   DISSCO.Project\shell\open\command
//                                   (Default)  = "<exe>" "%1"
//
// HKCU only — no admin required, no system-wide effect, and naturally scoped
// to the current user. Last-Release-launched wins.

constexpr const wchar_t* kProgId       = L"DISSCO.Project";
constexpr const wchar_t* kProgIdLabel  = L"DISSCO Project";
constexpr const wchar_t* kExtKey       = L"Software\\Classes\\.dissco";
constexpr const wchar_t* kProgIdKey    = L"Software\\Classes\\DISSCO.Project";
constexpr const wchar_t* kCommandKey   = L"Software\\Classes\\DISSCO.Project\\shell\\open\\command";

bool writeString(HKEY root, const wchar_t* subkey, const wchar_t* valueName, const wchar_t* value)
{
    HKEY hKey = nullptr;
    LONG rc = RegCreateKeyExW(root, subkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                              KEY_SET_VALUE, nullptr, &hKey, nullptr);
    if (rc != ERROR_SUCCESS) return false;
    const DWORD bytes = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
    rc = RegSetValueExW(hKey, valueName, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(value), bytes);
    RegCloseKey(hKey);
    return rc == ERROR_SUCCESS;
}

bool readString(HKEY root, const wchar_t* subkey, const wchar_t* valueName, std::wstring& out)
{
    HKEY hKey = nullptr;
    LONG rc = RegOpenKeyExW(root, subkey, 0, KEY_QUERY_VALUE, &hKey);
    if (rc != ERROR_SUCCESS) return false;
    DWORD type = 0;
    DWORD size = 0;
    rc = RegQueryValueExW(hKey, valueName, nullptr, &type, nullptr, &size);
    if (rc != ERROR_SUCCESS || type != REG_SZ || size == 0) {
        RegCloseKey(hKey);
        return false;
    }
    std::wstring buf(size / sizeof(wchar_t), L'\0');
    rc = RegQueryValueExW(hKey, valueName, nullptr, &type,
                          reinterpret_cast<BYTE*>(buf.data()), &size);
    RegCloseKey(hKey);
    if (rc != ERROR_SUCCESS) return false;
    // RegQueryValueExW may include a trailing NUL inside the string buffer.
    while (!buf.empty() && buf.back() == L'\0')
        buf.pop_back();
    out = std::move(buf);
    return true;
}

} // namespace

namespace dissco { namespace win {

void registerDisscoAssociation()
{
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString commandLine = QStringLiteral("\"%1\" \"%%1\"").arg(exePath);
    const std::wstring commandLineW = commandLine.toStdWString();

    // Idempotency check: if HKCU already maps .dissco to our ProgID and our
    // ProgID's open-command already invokes this exact exe, do nothing. This
    // keeps repeated launches from churning the registry.
    std::wstring existingProgId;
    std::wstring existingCommand;
    const bool extOk = readString(HKEY_CURRENT_USER, kExtKey, L"", existingProgId)
                       && existingProgId == kProgId;
    const bool cmdOk = readString(HKEY_CURRENT_USER, kCommandKey, L"", existingCommand)
                       && existingCommand == commandLineW;
    if (extOk && cmdOk)
        return;

    writeString(HKEY_CURRENT_USER, kExtKey,    L"", kProgId);
    writeString(HKEY_CURRENT_USER, kProgIdKey, L"", kProgIdLabel);
    writeString(HKEY_CURRENT_USER, kProgIdKey, L"FriendlyTypeName", kProgIdLabel);
    writeString(HKEY_CURRENT_USER, kCommandKey, L"", commandLineW.c_str());

    // Tell Explorer to refresh its association cache so the new mapping
    // takes effect without a logout.
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

}} // namespace dissco::win

#else  // !(_WIN32 && LASSIE_CLAIM_DISSCO)

namespace dissco { namespace win {

void registerDisscoAssociation() {}

}} // namespace dissco::win

#endif
