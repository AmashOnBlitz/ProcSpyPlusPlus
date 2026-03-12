#include "pch.h"
#include "HookMethods.h"
#include "messageQueue.h"

std::string HookMethods::Utility::DecodeAccess(DWORD access)
{
    std::string out;

    if (access & GENERIC_READ)        out += "GENERIC_READ|";
    if (access & GENERIC_WRITE)       out += "GENERIC_WRITE|";
    if (access & GENERIC_EXECUTE)     out += "GENERIC_EXECUTE|";
    if (access & GENERIC_ALL)         out += "GENERIC_ALL|";

    if (access & FILE_READ_DATA)      out += "READ_DATA|";
    if (access & FILE_WRITE_DATA)     out += "WRITE_DATA|";
    if (access & FILE_APPEND_DATA)    out += "APPEND|";

    if (access & FILE_READ_ATTRIBUTES)  out += "READ_ATTR|";
    if (access & FILE_WRITE_ATTRIBUTES) out += "WRITE_ATTR|";

    if (access & DELETE)              out += "DELETE|";

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

std::string HookMethods::Utility::File::GetPathFromHandle(HANDLE hFile)
{
    WCHAR path[MAX_PATH];

    DWORD result = GetFinalPathNameByHandleW(
        hFile,
        path,
        MAX_PATH,
        FILE_NAME_NORMALIZED
    );

    if (result == 0 || result > MAX_PATH)
        return "UNKNOWN_HANDLE";

    std::string finalPath = Utility::WStringToString(path);

    if (finalPath.rfind("\\\\?\\", 0) == 0)
    {
        finalPath = finalPath.substr(4);
    }

    if (finalPath.rfind("UNC\\", 0) == 0)
    {
        finalPath = "\\" + finalPath.substr(3);
    }

    return finalPath;
}