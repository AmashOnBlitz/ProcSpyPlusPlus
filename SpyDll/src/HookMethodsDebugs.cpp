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

std::string HookMethods::Utility::Window::getCreateWindowExDebugString(
    DWORD       dwExStyle,
    const void* lpClassName,
    const void* lpWindowName,
    DWORD       dwStyle,
    int         X,
    int         Y,
    int         nWidth,
    int         nHeight,
    HWND        hWndParent,
    HMENU       hMenu,
    HINSTANCE   hInstance,
    LPVOID      lpParam,
    HWND        result,
    bool        isWide
)
{
    std::string finalClass;
    std::string finalTitle;

    if (isWide) {
        LPCWSTR cls = static_cast<LPCWSTR>(lpClassName);
        LPCWSTR wnd = static_cast<LPCWSTR>(lpWindowName);
        if (cls) {
            if (reinterpret_cast<ULONG_PTR>(cls) <= 0xFFFF)
                finalClass = "ATOM:" + std::to_string(reinterpret_cast<WORD>(cls));
            else
                finalClass = Utility::WStringToString(cls);
        }
        finalTitle = wnd ? Utility::WStringToString(wnd) : "";
    }
    else {
        LPCSTR cls = static_cast<LPCSTR>(lpClassName);
        LPCSTR wnd = static_cast<LPCSTR>(lpWindowName);
        if (cls) {
            if (reinterpret_cast<ULONG_PTR>(cls) <= 0xFFFF)
                finalClass = "ATOM:" + std::to_string(reinterpret_cast<WORD>(cls));
            else
                finalClass = cls;
        }
        finalTitle = wnd ? std::string(wnd) : "";
    }

    std::ostringstream ss;
    ss << GetTrackStr("WINDOW CREATE")
        << " | " << (HookMethods::Window::WindowEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time     : " << Utility::GetTimestamp()
        << "\n    API      : " << (isWide ? "CreateWindowExW" : "CreateWindowExA")
        << "\n    Class    : " << finalClass
        << "\n    Title    : " << finalTitle
        << "\n    Style    : " << Utility::Window::DecodeWindowStyle(dwStyle)
        << "\n    ExStyle  : " << Utility::Window::DecodeWindowExStyle(dwExStyle)
        << "\n    Pos      : (" << X << ", " << Y << ")"
        << "\n    Size     : " << nWidth << "x" << nHeight
        << "\n    Parent   : " << reinterpret_cast<uintptr_t>(hWndParent)
        << "\n    Result   : " << reinterpret_cast<uintptr_t>(result);
    return ss.str();
}

std::string HookMethods::Utility::Network::getNetworkSendDebugString(
    SOCKET s,
    DWORD  bytesSent,
    int    flags,
    int    result,
    bool   allowed
)
{
    std::ostringstream ss;
    ss << GetTrackStr("NETWORK SEND")
        << " | " << (allowed ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : send"
        << "\n    Socket  : " << s
        << "\n    Flags   : 0x" << std::hex << flags << std::dec
        << "\n    Sent    : " << bytesSent
        << "\n    Result  : " << (result != SOCKET_ERROR ? "SUCCESS" : "ERROR");
    return ss.str();
}

std::string HookMethods::Utility::Network::getNetworkSendExDebugString(
    SOCKET s,
    DWORD  bufferCount,
    DWORD  bytesSent,
    DWORD  wsaFlags,
    bool   async,
    int    result,
    bool   allowed
)
{
    std::ostringstream ss;
    ss << GetTrackStr("NETWORK SEND EX")
        << " | " << (allowed ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : WSASend"
        << "\n    Socket  : " << s
        << "\n    Buffers : " << bufferCount
        << "\n    Flags   : 0x" << std::hex << wsaFlags << std::dec
        << "\n    Sent    : " << bytesSent
        << "\n    Mode    : " << (async ? "ASYNC" : "SYNC")
        << "\n    Result  : " << (result == 0 ? "SUCCESS" : "ERROR");
    return ss.str();
}

std::string HookMethods::Utility::Network::getNetworkReceiveDebugString(
    SOCKET s,
    DWORD  bytesRecvd,
    int    flags,
    int    result,
    bool   allowed
)
{
    std::ostringstream ss;
    ss << GetTrackStr("NETWORK RECEIVE")
        << " | " << (allowed ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : recv"
        << "\n    Socket  : " << s
        << "\n    Flags   : 0x" << std::hex << flags << std::dec
        << "\n    Recvd   : " << bytesRecvd
        << "\n    Result  : " << (result != SOCKET_ERROR ? "SUCCESS" : "ERROR");
    return ss.str();
}

std::string HookMethods::Utility::Network::getNetworkReceiveExDebugString(
    SOCKET s,
    DWORD  bufferCount,
    DWORD  bytesRecvd,
    bool   async,
    int    result,
    bool   allowed
)
{
    std::ostringstream ss;
    ss << GetTrackStr("NETWORK RECEIVE EX")
        << " | " << (allowed ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : WSARecv"
        << "\n    Socket  : " << s
        << "\n    Buffers : " << bufferCount
        << "\n    Recvd   : " << bytesRecvd
        << "\n    Mode    : " << (async ? "ASYNC" : "SYNC")
        << "\n    Result  : " << (result == 0 ? "SUCCESS" : "ERROR");
    return ss.str();
}

static std::string DecodeCreationFlags(DWORD flags)
{
    if (flags == 0) return "NONE";
    std::string out;
    if (flags & CREATE_SUSPENDED)          out += "SUSPENDED|";
    if (flags & STACK_SIZE_PARAM_IS_A_RESERVATION) out += "STACK_RESERVE|";
    if (!out.empty()) out.pop_back();
    return out;
}

std::string HookMethods::Utility::Thread::getCreateThreadDebugString(
    SIZE_T                dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID                lpParameter,
    DWORD                 dwCreationFlags,
    DWORD                 threadId,
    HANDLE                result
)
{
    std::ostringstream ss;
    ss << GetTrackStr("THREAD CREATE")
        << " | " << (HookMethods::Thread::ThreadEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time     : " << Utility::GetTimestamp()
        << "\n    API      : CreateThread"
        << "\n    Start    : " << reinterpret_cast<uintptr_t>(lpStartAddress)
        << "\n    Param    : " << reinterpret_cast<uintptr_t>(lpParameter)
        << "\n    Stack    : " << dwStackSize
        << "\n    Flags    : " << DecodeCreationFlags(dwCreationFlags)
        << "\n    TID      : " << threadId
        << "\n    Result   : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}

std::string HookMethods::Utility::Thread::getCreateRemoteThreadExDebugString(
    HANDLE                hProcess,
    SIZE_T                dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID                lpParameter,
    DWORD                 dwCreationFlags,
    DWORD                 threadId,
    HANDLE                result
)
{
    std::ostringstream ss;
    ss << GetTrackStr("THREAD CREATE REMOTE")
        << " | " << (HookMethods::Thread::ThreadEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time     : " << Utility::GetTimestamp()
        << "\n    API      : CreateRemoteThreadEx"
        << "\n    Process  : " << reinterpret_cast<uintptr_t>(hProcess)
        << "\n    Start    : " << reinterpret_cast<uintptr_t>(lpStartAddress)
        << "\n    Param    : " << reinterpret_cast<uintptr_t>(lpParameter)
        << "\n    Stack    : " << dwStackSize
        << "\n    Flags    : " << DecodeCreationFlags(dwCreationFlags)
        << "\n    TID      : " << threadId
        << "\n    Result   : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}

static std::string DecodeDLLFlags(DWORD flags)
{
    if (flags == 0) return "NONE";
    std::string out;
    if (flags & DONT_RESOLVE_DLL_REFERENCES)        out += "DONT_RESOLVE|";
    if (flags & LOAD_LIBRARY_AS_DATAFILE)           out += "AS_DATAFILE|";
    if (flags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) out += "AS_DATAFILE_EX|";
    if (flags & LOAD_LIBRARY_AS_IMAGE_RESOURCE)     out += "AS_IMAGE|";
    if (flags & LOAD_LIBRARY_SEARCH_APPLICATION_DIR) out += "SEARCH_APPDIR|";
    if (flags & LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)   out += "SEARCH_DEFAULT|";
    if (flags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)   out += "SEARCH_DLLDIR|";
    if (flags & LOAD_LIBRARY_SEARCH_SYSTEM32)        out += "SEARCH_SYS32|";
    if (flags & LOAD_LIBRARY_SEARCH_USER_DIRS)       out += "SEARCH_USER|";
    if (!out.empty()) out.pop_back();
    return out;
}

std::string HookMethods::Utility::DLL::getDLLLoadDebugString(
    const void* lpLibFileName,
    DWORD       dwFlags,
    HMODULE     result,
    bool        isWide,
    bool        isEx
)
{
    std::string finalPath;
    if (isWide)
        finalPath = lpLibFileName ? Utility::WStringToString(static_cast<LPCWSTR>(lpLibFileName)) : "";
    else
        finalPath = lpLibFileName ? static_cast<LPCSTR>(lpLibFileName) : "";

    std::ostringstream ss;
    ss << GetTrackStr("DLL LOAD")
        << " | " << (HookMethods::DLL::DLLEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : " << (isEx ? (isWide ? "LoadLibraryExW" : "LoadLibraryExA")
                                  : (isWide ? "LoadLibraryW" : "LoadLibraryA"))
        << "\n    Path    : " << finalPath;
    if (isEx)
        ss << "\n    Flags   : " << DecodeDLLFlags(dwFlags);
    ss << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}

std::string HookMethods::Utility::Clipboard::DecodeClipboardFormat(UINT fmt)
{
    switch (fmt) {
    case CF_TEXT:         return "CF_TEXT";
    case CF_BITMAP:       return "CF_BITMAP";
    case CF_METAFILEPICT: return "CF_METAFILEPICT";
    case CF_SYLK:         return "CF_SYLK";
    case CF_DIF:          return "CF_DIF";
    case CF_TIFF:         return "CF_TIFF";
    case CF_OEMTEXT:      return "CF_OEMTEXT";
    case CF_DIB:          return "CF_DIB";
    case CF_PALETTE:      return "CF_PALETTE";
    case CF_PENDATA:      return "CF_PENDATA";
    case CF_RIFF:         return "CF_RIFF";
    case CF_WAVE:         return "CF_WAVE";
    case CF_UNICODETEXT:  return "CF_UNICODETEXT";
    case CF_ENHMETAFILE:  return "CF_ENHMETAFILE";
    case CF_HDROP:        return "CF_HDROP";
    case CF_LOCALE:       return "CF_LOCALE";
    case CF_DIBV5:        return "CF_DIBV5";
    default:              return "CUSTOM(" + std::to_string(fmt) + ")";
    }
}

std::string HookMethods::Utility::Clipboard::getOpenClipboardDebugString(HWND hWnd, BOOL result)
{
    std::ostringstream ss;
    ss << GetTrackStr("CLIPBOARD OPEN")
        << " | " << (HookMethods::Clipboard::ClipboardEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : OpenClipboard"
        << "\n    HWND    : " << reinterpret_cast<uintptr_t>(hWnd)
        << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}

std::string HookMethods::Utility::Clipboard::getGetClipboardDataDebugString(UINT uFormat, HANDLE result)
{
    std::ostringstream ss;
    ss << GetTrackStr("CLIPBOARD READ")
        << " | " << (HookMethods::Clipboard::ClipboardEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : GetClipboardData"
        << "\n    Format  : " << DecodeClipboardFormat(uFormat)
        << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}

std::string HookMethods::Utility::Clipboard::getSetClipboardDataDebugString(UINT uFormat, HANDLE hMem, HANDLE result)
{
    std::ostringstream ss;
    ss << GetTrackStr("CLIPBOARD WRITE")
        << " | " << (HookMethods::Clipboard::ClipboardEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : SetClipboardData"
        << "\n    Format  : " << DecodeClipboardFormat(uFormat)
        << "\n    Data    : " << reinterpret_cast<uintptr_t>(hMem)
        << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}

std::string HookMethods::Utility::Clipboard::getEmptyClipboardDebugString(BOOL result)
{
    std::ostringstream ss;
    ss << GetTrackStr("CLIPBOARD CLEAR")
        << " | " << (HookMethods::Clipboard::ClipboardEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : EmptyClipboard"
        << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}

std::string HookMethods::Utility::Screenshot::DecodeBitBltRop(DWORD rop)
{
    switch (rop) {
    case SRCCOPY:    return "SRCCOPY";
    case SRCPAINT:   return "SRCPAINT";
    case SRCAND:     return "SRCAND";
    case SRCERASE:   return "SRCERASE";
    case SRCINVERT:  return "SRCINVERT";
    case PATCOPY:    return "PATCOPY";
    case PATPAINT:   return "PATPAINT";
    case PATINVERT:  return "PATINVERT";
    case DSTINVERT:  return "DSTINVERT";
    case BLACKNESS:  return "BLACKNESS";
    case WHITENESS:  return "WHITENESS";
    case CAPTUREBLT: return "CAPTUREBLT";
    default:         return "0x" + [rop] { std::ostringstream o; o << std::hex << rop; return o.str(); }();
    }
}

std::string HookMethods::Utility::Screenshot::getBitBltDebugString(
    int   xDest,
    int   yDest,
    int   nWidth,
    int   nHeight,
    int   xSrc,
    int   ySrc,
    DWORD rop,
    BOOL  result
)
{
    std::ostringstream ss;
    ss << GetTrackStr("SCREENSHOT CAPTURE")
        << " | " << (HookMethods::Screenshot::ScreenshotEnabled ? "ALLOWED" : "BLOCKED")
        << "\n    Time    : " << Utility::GetTimestamp()
        << "\n    API     : BitBlt"
        << "\n    Dest    : (" << xDest << ", " << yDest << ")"
        << "\n    Size    : " << nWidth << "x" << nHeight
        << "\n    Src     : (" << xSrc << ", " << ySrc << ")"
        << "\n    ROP     : " << DecodeBitBltRop(rop)
        << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
    return ss.str();
}