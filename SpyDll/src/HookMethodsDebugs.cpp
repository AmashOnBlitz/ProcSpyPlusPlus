#include "pch.h"
#include "HookMethods.h"
#include "messageQueue.h"

std::string HookMethods::Utility::File::getFileCreateDebugString(
    const void* fileName,
    DWORD access,
    DWORD shareMode,
    LPSECURITY_ATTRIBUTES secAttr,
    DWORD creation,
    DWORD flags,
    HANDLE templateFile,
    bool isWide
)
{
    std::string finalPath;

    if (isWide)
    {
        LPCWSTR name = static_cast<LPCWSTR>(fileName);

        WCHAR fullPath[MAX_PATH];
        GetFullPathNameW(name, MAX_PATH, fullPath, nullptr);

        finalPath = Utility::WStringToString(fullPath);
    }
    else
    {
        LPCSTR name = static_cast<LPCSTR>(fileName);

        CHAR fullPath[MAX_PATH];
        GetFullPathNameA(name, MAX_PATH, fullPath, nullptr);

        finalPath = fullPath;

        if (finalPath.rfind("\\\\?\\", 0) == 0)
        {
            finalPath = finalPath.substr(4);
        }
        if (finalPath.rfind("\\?\\", 0) == 0)
        {
            finalPath = finalPath.substr(4);
        }
        if (finalPath.rfind("UNC\\", 0) == 0)
        {
            finalPath = "\\" + finalPath.substr(3);
        }
    }

    std::ostringstream ss;
    bool fileExisted = (GetFileAttributesA(finalPath.c_str()) != INVALID_FILE_ATTRIBUTES);
    std::string intent = Utility::DecodeFileIntent(access, creation, flags, fileExisted);
    ss << GetTrackStr("FILE " + intent)
        << " | " << (HookMethods::File::Creation::CreateEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API        : " << (isWide ? "CreateFileW" : "CreateFileA")
        << "\n    Path       : " << finalPath
        << "\n    Access     : " << Utility::DecodeAccess(access)
        << "\n    Share      : " << Utility::DecodeShareMode(shareMode)
        << "\n    Disposition: " << Utility::DecodeCreation(creation)
        << "\n    Flags      : " << Utility::DecodeFlags(flags);

    return ss.str();
}

std::string HookMethods::Utility::File::getFileReadDebugString(
    HANDLE hFile,
    LPVOID buffer,
    DWORD bytesToRead,
    LPDWORD bytesRead,
    LPOVERLAPPED overlapped
)
{
    std::ostringstream ss;

    DWORD read = (bytesRead) ? *bytesRead : 0;

    ss << GetTrackStr("FILE READ")
        << " | " << (HookMethods::File::Read::ReadEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API        : ReadFile"
        << "\n    Handle     : " << hFile
        << "\n    Path       : " << Utility::File::GetPathFromHandle(hFile)
        << "\n    Bytes Req  : " << bytesToRead
        << "\n    Bytes Read : " << read
        << "\n    Mode       : " << (overlapped ? "ASYNC" : "SYNC");

    return ss.str();
}

std::string HookMethods::Utility::File::getFileReadExDebugString(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    std::ostringstream ss;

    ss << GetTrackStr("FILE READ EX")
        << " | " << (HookMethods::File::Read::ReadEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API        : ReadFileEx"
        << "\n    Handle     : " << hFile
        << "\n    Path       : " << Utility::File::GetPathFromHandle(hFile)
        << "\n    Bytes Req  : " << nNumberOfBytesToRead
        << "\n    Completion : " << (lpCompletionRoutine ? "PROVIDED" : "NONE")
        << "\n    Mode       : ASYNC";

    return ss.str();
}

std::string HookMethods::Utility::File::getFileWriteDebugString(
    HANDLE hFile,
    LPCVOID buffer,
    DWORD bytesToWrite,
    LPDWORD bytesWritten,
    LPOVERLAPPED overlapped,
    bool allowed
)
{
    std::ostringstream ss;

    DWORD written = (allowed) ? *bytesWritten : 0;

    ss << GetTrackStr("FILE WRITE")
        << " | " << (HookMethods::File::Write::WriteEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API        : WriteFile"
        << "\n    Handle     : " << hFile
        << "\n    Path       : " << Utility::File::GetPathFromHandle(hFile)
        << "\n    Bytes Req  : " << bytesToWrite
        << "\n    Bytes Done : " << written
        << "\n    Mode       : " << (overlapped ? "ASYNC" : "SYNC");

    return ss.str();
}

std::string HookMethods::Utility::File::getFileWriteExDebugString(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    std::ostringstream ss;

    ss << GetTrackStr("FILE WRITE EX")
        << " | " << (HookMethods::File::Write::WriteEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API        : WriteFileEx"
        << "\n    Handle     : " << hFile
        << "\n    Path       : " << Utility::File::GetPathFromHandle(hFile)
        << "\n    Bytes Req  : " << nNumberOfBytesToWrite
        << "\n    Completion : " << (lpCompletionRoutine ? "PROVIDED" : "NONE")
        << "\n    Mode       : ASYNC";

    return ss.str();
}

std::string HookMethods::Utility::MsgBox::getMsgBoxDebugString(
    HWND hWnd,
    const void* text,
    const void* caption,
    UINT type,
    bool isWide
)
{
    std::string finalText;
    std::string finalCaption;
    if (isWide) {
        finalText = Utility::WStringToString(static_cast<LPCWSTR>(text));
        finalCaption = Utility::WStringToString(static_cast<LPCWSTR>(caption));
    }
    else {
        finalText = static_cast<LPCSTR>(text) ? static_cast<LPCSTR>(text) : "";
        finalCaption = static_cast<LPCSTR>(caption) ? static_cast<LPCSTR>(caption) : "";
    }

    std::string typeStr;
    switch (type & 0xF) {
    case MB_OK:                typeStr = "OK";               break;
    case MB_OKCANCEL:          typeStr = "OK|CANCEL";        break;
    case MB_ABORTRETRYIGNORE:  typeStr = "ABORT|RETRY|IGN";  break;
    case MB_YESNOCANCEL:       typeStr = "YES|NO|CANCEL";    break;
    case MB_YESNO:             typeStr = "YES|NO";           break;
    case MB_RETRYCANCEL:       typeStr = "RETRY|CANCEL";     break;
    default:                   typeStr = "UNKNOWN(" + std::to_string(type & 0xF) + ")"; break;
    }

    std::ostringstream ss;
    ss << GetTrackStr("MSGBOX")
        << " | " << (HookMethods::MsgBox::MsgBoxEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : " << (isWide ? "MessageBoxW" : "MessageBoxA")
        << "\n    HWND    : " << reinterpret_cast<uintptr_t>(hWnd)
        << "\n    Caption : " << finalCaption
        << "\n    Text    : " << finalText
        << "\n    Type    : " << typeStr;
    return ss.str();
}

std::string HookMethods::Utility::Registry::getRegReadDebugString(
    HKEY hKey,
    const void* lpValueName,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LONG result,
    bool isWide
)
{
    std::string finalValueName;
    if (isWide)
        finalValueName = Utility::WStringToString(static_cast<LPCWSTR>(lpValueName));
    else
        finalValueName = static_cast<LPCSTR>(lpValueName) ? static_cast<LPCSTR>(lpValueName) : "";

    DWORD dataSize = (lpcbData) ? *lpcbData : 0;
    DWORD regType = (lpType) ? *lpType : REG_NONE;

    std::ostringstream ss;
    ss << GetTrackStr("REGISTRY READ")
        << " | " << (HookMethods::Registry::Read::ReadEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API        : " << (isWide ? "RegQueryValueExW" : "RegQueryValueExA")
        << "\n    Key        : " << Utility::Registry::GetKeyNameFromHandle(hKey)
        << "\n    Value      : " << finalValueName
        << "\n    Type       : " << Utility::Registry::DecodeRegType(regType)
        << "\n    Data Size  : " << dataSize
        << "\n    Result     : " << (result == ERROR_SUCCESS ? "SUCCESS" : "FAILED(" + std::to_string(result) + ")");
    return ss.str();
}

std::string HookMethods::Utility::Registry::getRegWriteDebugString(
    HKEY hKey,
    const void* lpValueName,
    DWORD dwType,
    const BYTE* lpData,
    DWORD cbData,
    LONG result,
    bool isWide
)
{
    std::string finalValueName;
    if (isWide)
        finalValueName = Utility::WStringToString(static_cast<LPCWSTR>(lpValueName));
    else
        finalValueName = static_cast<LPCSTR>(lpValueName) ? static_cast<LPCSTR>(lpValueName) : "";

    std::ostringstream ss;
    ss << GetTrackStr("REGISTRY WRITE")
        << " | " << (HookMethods::Registry::Write::WriteEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API        : " << (isWide ? "RegSetValueExW" : "RegSetValueExA")
        << "\n    Key        : " << Utility::Registry::GetKeyNameFromHandle(hKey)
        << "\n    Value      : " << finalValueName
        << "\n    Type       : " << Utility::Registry::DecodeRegType(dwType)
        << "\n    Data Size  : " << cbData
        << "\n    Result     : " << (result == ERROR_SUCCESS ? "SUCCESS" : "FAILED(" + std::to_string(result) + ")");
    return ss.str();
}

static std::string DecodeTemplateNameA(LPCSTR lpTemplateName)
{
    if (!lpTemplateName) return "NULL";
    if (IS_INTRESOURCE(lpTemplateName))
        return "ID:" + std::to_string(reinterpret_cast<WORD>(lpTemplateName));
    return std::string(lpTemplateName);
}

static std::string DecodeTemplateNameW(LPCWSTR lpTemplateName)
{
    if (!lpTemplateName) return "NULL";
    if (IS_INTRESOURCE(lpTemplateName))
        return "ID:" + std::to_string(reinterpret_cast<WORD>(lpTemplateName));
    return HookMethods::Utility::WStringToString(lpTemplateName);
}

std::string HookMethods::Utility::Dialog::getDialogBoxParamDebugString(
    HINSTANCE   hInstance,
    const void* lpTemplateName,
    HWND        hWndParent,
    DLGPROC     lpDialogFunc,
    LPARAM      dwInitParam,
    bool        isWide
)
{
    std::string finalTemplate = isWide
        ? DecodeTemplateNameW(static_cast<LPCWSTR>(lpTemplateName))
        : DecodeTemplateNameA(static_cast<LPCSTR>(lpTemplateName));

    std::ostringstream ss;
    ss << GetTrackStr("DIALOG BOX")
        << " | " << (HookMethods::Dialog::DialogEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time     : " << Utility::GetTimestamp()
        << "\n    API      : " << (isWide ? "DialogBoxParamW" : "DialogBoxParamA")
        << "\n    Template : " << finalTemplate
        << "\n    Parent   : " << reinterpret_cast<uintptr_t>(hWndParent)
        << "\n    DlgProc  : " << reinterpret_cast<uintptr_t>(lpDialogFunc)
        << "\n    InitParam: " << dwInitParam
        << "\n    Mode     : Modal";
    return ss.str();
}

std::string HookMethods::Utility::Dialog::getCreateDialogParamDebugString(
    HINSTANCE   hInstance,
    const void* lpTemplateName,
    HWND        hWndParent,
    DLGPROC     lpDialogFunc,
    LPARAM      dwInitParam,
    bool        isWide
)
{
    std::string finalTemplate = isWide
        ? DecodeTemplateNameW(static_cast<LPCWSTR>(lpTemplateName))
        : DecodeTemplateNameA(static_cast<LPCSTR>(lpTemplateName));

    std::ostringstream ss;
    ss << GetTrackStr("DIALOG CREATE")
        << " | " << (HookMethods::Dialog::DialogEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time     : " << Utility::GetTimestamp()
        << "\n    API      : " << (isWide ? "CreateDialogParamW" : "CreateDialogParamA")
        << "\n    Template : " << finalTemplate
        << "\n    Parent   : " << reinterpret_cast<uintptr_t>(hWndParent)
        << "\n    DlgProc  : " << reinterpret_cast<uintptr_t>(lpDialogFunc)
        << "\n    InitParam: " << dwInitParam
        << "\n    Mode     : Modeless";
    return ss.str();
}

std::string HookMethods::Utility::Dialog::getTaskDialogDebugString(
    HWND                           hwndOwner,
    PCWSTR                         pszWindowTitle,
    PCWSTR                         pszMainInstruction,
    PCWSTR                         pszContent,
    TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
    PCWSTR                         pszIcon
)
{
    std::string finalTitle = pszWindowTitle ? Utility::WStringToString(pszWindowTitle) : "";
    std::string finalInstruction = pszMainInstruction ? Utility::WStringToString(pszMainInstruction) : "";
    std::string finalContent = pszContent ? Utility::WStringToString(pszContent) : "";
    std::string finalIcon = IS_INTRESOURCE(pszIcon)
        ? "ID:" + std::to_string(reinterpret_cast<WORD>(pszIcon))
        : (pszIcon ? Utility::WStringToString(pszIcon) : "NONE");

    std::ostringstream ss;
    ss << GetTrackStr("TASK DIALOG")
        << " | " << (HookMethods::Dialog::DialogEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time        : " << Utility::GetTimestamp()
        << "\n    API         : TaskDialog"
        << "\n    Owner       : " << reinterpret_cast<uintptr_t>(hwndOwner)
        << "\n    Title       : " << finalTitle
        << "\n    Instruction : " << finalInstruction
        << "\n    Content     : " << finalContent
        << "\n    Icon        : " << finalIcon
        << "\n    Buttons     : 0x" << std::hex << dwCommonButtons << std::dec;
    return ss.str();
}

std::string HookMethods::Utility::Dialog::getTaskDialogIndirectDebugString(
    const TASKDIALOGCONFIG* pTaskConfig
)
{
    std::ostringstream ss;
    ss << GetTrackStr("TASK DIALOG INDIRECT")
        << " | " << (HookMethods::Dialog::DialogEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time        : " << Utility::GetTimestamp()
        << "\n    API         : TaskDialogIndirect";

    if (pTaskConfig) {
        std::string finalTitle = pTaskConfig->pszWindowTitle ? Utility::WStringToString(pTaskConfig->pszWindowTitle) : "";
        std::string finalInstruction = pTaskConfig->pszMainInstruction ? Utility::WStringToString(pTaskConfig->pszMainInstruction) : "";
        std::string finalContent = pTaskConfig->pszContent ? Utility::WStringToString(pTaskConfig->pszContent) : "";

        ss << "\n    Owner       : " << reinterpret_cast<uintptr_t>(pTaskConfig->hwndParent)
            << "\n    Title       : " << finalTitle
            << "\n    Instruction : " << finalInstruction
            << "\n    Content     : " << finalContent
            << "\n    Buttons     : 0x" << std::hex << pTaskConfig->dwCommonButtons << std::dec
            << "\n    Flags       : 0x" << std::hex << pTaskConfig->dwFlags << std::dec;
    }
    else {
        ss << "\n    Config      : NULL";
    }

    return ss.str();
}