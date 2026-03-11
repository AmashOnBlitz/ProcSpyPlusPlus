#include "pch.h"
#include "HookMethods.h"
#include "messageQueue.h"

HANDLE WINAPI HookMethods::File::Write::CreateFileWHook(
    LPCWSTR fileName,
    DWORD access,
    DWORD shareMode,
    LPSECURITY_ATTRIBUTES secAttr,
    DWORD creation,
    DWORD flags,
    HANDLE templateFile
)
{
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileWriteDebugString(
                fileName,
                access,
                shareMode,
                secAttr,
                creation,
                flags,
                templateFile,
                true
            )
        );
    }
    if (WriteEnabled) {
        return OriginalCreateFileW(
            fileName,
            access,
            shareMode,
            secAttr,
            creation,
            flags,
            templateFile
        );
    }
    else {
        return HANDLE{};
    }
}

HANDLE WINAPI HookMethods::File::Write::CreateFileAHook(
    LPCSTR fileName,
    DWORD access,
    DWORD shareMode,
    LPSECURITY_ATTRIBUTES secAttr,
    DWORD creation,
    DWORD flags,
    HANDLE templateFile
)
{
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileWriteDebugString(
                fileName,
                access,
                shareMode,
                secAttr,
                creation,
                flags,
                templateFile,
                false
            )
        );
    }
    if (WriteEnabled) {
        return OriginalCreateFileA(
            fileName,
            access,
            shareMode,
            secAttr,
            creation,
            flags,
            templateFile
        );
    }
    else {
        return HANDLE{};
    }
}

std::string HookMethods::Utility::WStringToString(LPCWSTR wstr)
{
    if (!wstr) return "";

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0,
        wstr, -1,
        NULL, 0,
        NULL, NULL
    );

    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr, -1,
        &str[0],
        sizeNeeded,
        NULL, NULL
    );

    return str;
}

std::string HookMethods::Utility::DecodeCreation(DWORD creation)
{
    switch (creation) {
    case CREATE_NEW:        return "CREATE_NEW";
    case CREATE_ALWAYS:     return "CREATE_ALWAYS";
    case OPEN_EXISTING:     return "OPEN_EXISTING";
    case OPEN_ALWAYS:       return "OPEN_ALWAYS";
    case TRUNCATE_EXISTING: return "TRUNCATE_EXISTING";
    default:                return "UNKNOWN(" + std::to_string(creation) + ")";
    }
}

std::string HookMethods::Utility::DecodeShareMode(DWORD share)
{
    if (share == 0) return "EXCLUSIVE";
    std::string out;
    if (share & FILE_SHARE_READ)   out += "READ|";
    if (share & FILE_SHARE_WRITE)  out += "WRITE|";
    if (share & FILE_SHARE_DELETE) out += "DELETE|";
    if (!out.empty()) out.pop_back();
    return out;
}

std::string HookMethods::Utility::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm{};
    localtime_s(&tm, &time);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S");
    return ss.str();
}

std::string HookMethods::Utility::DecodeAccess(DWORD access)
{
    std::string out;
    if (access & GENERIC_READ)         out += "READ|";
    if (access & GENERIC_WRITE)        out += "WRITE|";
    if (access & GENERIC_EXECUTE)      out += "EXECUTE|";
    if (access & GENERIC_ALL)          out += "ALL|";
    if (access & FILE_APPEND_DATA)     out += "APPEND|";
    if (access & DELETE)               out += "DELETE|";
    if (!out.empty()) out.pop_back();  
    return out.empty() ? "NONE" : out;
}

std::string HookMethods::Utility::DecodeFlags(DWORD flags)
{
    {
        std::string out;
        if (flags & FILE_FLAG_WRITE_THROUGH)  out += "WRITE_THROUGH|";
        if (flags & FILE_FLAG_OVERLAPPED)     out += "OVERLAPPED|";
        if (flags & FILE_FLAG_NO_BUFFERING)   out += "NO_BUFFERING|";
        if (flags & FILE_FLAG_RANDOM_ACCESS)  out += "RANDOM_ACCESS|";
        if (flags & FILE_FLAG_SEQUENTIAL_SCAN)out += "SEQUENTIAL|";
        if (flags & FILE_FLAG_DELETE_ON_CLOSE)out += "DELETE_ON_CLOSE|";
        if (flags & FILE_ATTRIBUTE_HIDDEN)    out += "HIDDEN|";
        if (flags & FILE_ATTRIBUTE_READONLY)  out += "READONLY|";
        if (flags & FILE_ATTRIBUTE_TEMPORARY) out += "TEMPORARY|";
        if (!out.empty()) out.pop_back();
        return out.empty() ? "NORMAL" : out;
    }
}

std::string HookMethods::Utility::File::getFileWriteDebugString(
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
    }

    std::ostringstream ss;

    ss << GetTrackStr("FILE WRITE")
        << " | " << (HookMethods::File::Write::WriteEnabled ? "ALLOWED" : "BLOCKED") 
        << "\n    API        : " << (isWide ? "CreateFileW" : "CreateFileA")
        << "\n    Path       : " << finalPath
        << "\n    Access     : " << Utility::DecodeAccess(access)
        << "\n    Share      : " << Utility::DecodeShareMode(shareMode)
        << "\n    Disposition: " << Utility::DecodeCreation(creation)
        << "\n    Flags      : " << Utility::DecodeFlags(flags);

    return ss.str();
}

int WINAPI HookMethods::MsgBox::MessageBoxAHook(HWND hWnd, LPCSTR text, LPCSTR caption, UINT type)
{
    std::cout << "MsgBox W called\n";
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::MsgBox::getMsgBoxDebugString(hWnd, text, caption, type, false)
        );
    }
    if (MsgBoxEnabled) {
        std::cout << "MsgBox W enabled\n";
        return OriginalMessageBoxA(hWnd, text, caption, type);
    }
    else {
        return 0;
    }
}

int WINAPI HookMethods::MsgBox::MessageBoxWHook(HWND hWnd, LPCWSTR text, LPCWSTR caption, UINT type)
{
    std::cout << "MsgBox W called\n";
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::MsgBox::getMsgBoxDebugString(hWnd, text, caption, type, true)
        );
    }
    if (MsgBoxEnabled) {
        std::cout << "MsgBox W enabled\n";
        return OriginalMessageBoxW(hWnd, text, caption, type);
    }
    else {
        return 0;
    }
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
        << "\n    API     : " << (isWide ? "MessageBoxW" : "MessageBoxA")
        << "\n    HWND    : " << reinterpret_cast<uintptr_t>(hWnd)
        << "\n    Caption : " << finalCaption
        << "\n    Text    : " << finalText
        << "\n    Type    : " << typeStr;
    return ss.str();
}