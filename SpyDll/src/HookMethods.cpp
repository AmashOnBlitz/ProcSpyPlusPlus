#include "pch.h"
#include "HookMethods.h"
#include "messageQueue.h"

HANDLE WINAPI HookMethods::File::Creation::CreateFileWHook(
    LPCWSTR fileName,
    DWORD access,
    DWORD shareMode,
    LPSECURITY_ATTRIBUTES secAttr,
    DWORD creation,
    DWORD flags,
    HANDLE templateFile
)
{
    HANDLE result = INVALID_HANDLE_VALUE;

    if (CreateEnabled) {
        result = OriginalCreateFileW(
            fileName,
            access,
            shareMode,
            secAttr,
            creation,
            flags,
            templateFile
        );
    }

    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileCreateDebugString(
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

    return result;
}

HANDLE WINAPI HookMethods::File::Creation::CreateFileAHook(
    LPCSTR fileName,
    DWORD access,
    DWORD shareMode,
    LPSECURITY_ATTRIBUTES secAttr,
    DWORD creation,
    DWORD flags,
    HANDLE templateFile
)
{
    HANDLE result = INVALID_HANDLE_VALUE;
    if (CreateEnabled) {
        result = OriginalCreateFileA(
            fileName,
            access,
            shareMode,
            secAttr,
            creation,
            flags,
            templateFile
        );
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileCreateDebugString(
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
    return result;
}

int WINAPI HookMethods::MsgBox::MessageBoxAHook(HWND hWnd, LPCSTR text, LPCSTR caption, UINT type)
{
    int result = 0;
    if (MsgBoxEnabled) {
        result = OriginalMessageBoxA(hWnd, text, caption, type);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::MsgBox::getMsgBoxDebugString(hWnd, text, caption, type, false)
        );
    }
    return result;
}

int WINAPI HookMethods::MsgBox::MessageBoxWHook(HWND hWnd, LPCWSTR text, LPCWSTR caption, UINT type)
{
    int result = 0;
    if (MsgBoxEnabled) {
        result = OriginalMessageBoxW(hWnd, text, caption, type);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::MsgBox::getMsgBoxDebugString(hWnd, text, caption, type, true)
        );
    }
    return result;
}

INT_PTR WINAPI HookMethods::Dialog::DialogBoxParamAHook(
    HINSTANCE hInstance,
    LPCSTR    lpTemplateName,
    HWND      hWndParent,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam
)
{
    INT_PTR result = -1;
    if (DialogEnabled) {
        result = OriginalDialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Dialog::getDialogBoxParamDebugString(
                hInstance,
                lpTemplateName,
                hWndParent,
                lpDialogFunc,
                dwInitParam,
                false
            )
        );
    }
    return result;
}

INT_PTR WINAPI HookMethods::Dialog::DialogBoxParamWHook(
    HINSTANCE hInstance,
    LPCWSTR   lpTemplateName,
    HWND      hWndParent,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam
)
{
    INT_PTR result = -1;
    if (DialogEnabled) {
        result = OriginalDialogBoxParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Dialog::getDialogBoxParamDebugString(
                hInstance,
                lpTemplateName,
                hWndParent,
                lpDialogFunc,
                dwInitParam,
                true
            )
        );
    }
    return result;
}

HWND WINAPI HookMethods::Dialog::CreateDialogParamAHook(
    HINSTANCE hInstance,
    LPCSTR    lpTemplateName,
    HWND      hWndParent,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam
)
{
    HWND result = NULL;
    if (DialogEnabled) {
        result = OriginalCreateDialogParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Dialog::getCreateDialogParamDebugString(
                hInstance,
                lpTemplateName,
                hWndParent,
                lpDialogFunc,
                dwInitParam,
                false
            )
        );
    }
    return result;
}

HWND WINAPI HookMethods::Dialog::CreateDialogParamWHook(
    HINSTANCE hInstance,
    LPCWSTR   lpTemplateName,
    HWND      hWndParent,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam
)
{
    HWND result = NULL;
    if (DialogEnabled) {
        result = OriginalCreateDialogParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Dialog::getCreateDialogParamDebugString(
                hInstance,
                lpTemplateName,
                hWndParent,
                lpDialogFunc,
                dwInitParam,
                true
            )
        );
    }
    return result;
}

HRESULT WINAPI HookMethods::Dialog::TaskDialogHook(
    HWND                           hwndOwner,
    HINSTANCE                      hInstance,
    PCWSTR                         pszWindowTitle,
    PCWSTR                         pszMainInstruction,
    PCWSTR                         pszContent,
    TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
    PCWSTR                         pszIcon,
    int* pnButton
)
{
    HRESULT result = E_FAIL;
    if (DialogEnabled) {
        result = OriginalTaskDialog(hwndOwner, hInstance, pszWindowTitle, pszMainInstruction, pszContent, dwCommonButtons, pszIcon, pnButton);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Dialog::getTaskDialogDebugString(
                hwndOwner,
                pszWindowTitle,
                pszMainInstruction,
                pszContent,
                dwCommonButtons,
                pszIcon
            )
        );
    }
    return result;
}

HRESULT WINAPI HookMethods::Dialog::TaskDialogIndirectHook(
    const TASKDIALOGCONFIG* pTaskConfig,
    int* pnButton,
    int* pnRadioButton,
    BOOL* pfVerificationFlagChecked
)
{
    HRESULT result = E_FAIL;
    if (DialogEnabled) {
        result = OriginalTaskDialogIndirect(pTaskConfig, pnButton, pnRadioButton, pfVerificationFlagChecked);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Dialog::getTaskDialogIndirectDebugString(pTaskConfig)
        );
    }
    return result;
}

BOOL WINAPI HookMethods::File::Read::ReadFileHook(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
)
{
    if (GetFileType(hFile) == FILE_TYPE_PIPE) {
        return OriginalReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }

    BOOL result = FALSE;
    if (ReadEnabled) {
        result = OriginalReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileReadDebugString(
                hFile,
                lpBuffer,
                nNumberOfBytesToRead,
                lpNumberOfBytesRead,
                lpOverlapped
            )
        );
    }
    return result;
}

BOOL WINAPI HookMethods::File::Read::ReadFileExHook(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    if (GetFileType(hFile) == FILE_TYPE_PIPE) {
        return OriginalReadFileEx(hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, lpCompletionRoutine);
    }

    BOOL result = FALSE;
    if (ReadEnabled) {
        result = OriginalReadFileEx(hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, lpCompletionRoutine);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileReadExDebugString(
                hFile,
                lpBuffer,
                nNumberOfBytesToRead,
                lpOverlapped,
                lpCompletionRoutine
            )
        );
    }
    return result;
}

BOOL WINAPI HookMethods::File::Write::WriteFileHook(
    HANDLE       hFile,
    LPCVOID      lpBuffer,
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
)
{
    if (GetFileType(hFile) == FILE_TYPE_PIPE) {
        return OriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }

    BOOL result = FALSE;

    if (WriteEnabled) {
        result = OriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }

    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileWriteDebugString(
                hFile,
                lpBuffer,
                nNumberOfBytesToWrite,
                lpNumberOfBytesWritten,
                lpOverlapped,
                WriteEnabled
            )
        );
    }

    return result;
}

BOOL WINAPI HookMethods::File::Write::WriteFileExHook(
    HANDLE       hFile,
    LPCVOID      lpBuffer,
    DWORD        nNumberOfBytesToWrite,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    if (GetFileType(hFile) == FILE_TYPE_PIPE) {
        return OriginalWriteFileEx(hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped, lpCompletionRoutine);
    }

    BOOL result = FALSE;

    if (WriteEnabled) {
        result = OriginalWriteFileEx(hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped, lpCompletionRoutine);
    }

    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileWriteExDebugString(
                hFile,
                lpBuffer,
                nNumberOfBytesToWrite,
                lpOverlapped,
                lpCompletionRoutine
            )
        );
    }

    return result;
}

LONG WINAPI HookMethods::Registry::Read::RegQueryValueExAHook(
    HKEY    hKey,
    LPCSTR  lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
)
{
    LONG result = ERROR_ACCESS_DENIED;
    if (ReadEnabled) {
        result = OriginalRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Registry::getRegReadDebugString(
                hKey,
                lpValueName,
                lpType,
                lpData,
                lpcbData,
                result,
                false
            )
        );
    }
    return result;
}

LONG WINAPI HookMethods::Registry::Read::RegQueryValueExWHook(
    HKEY    hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
)
{
    LONG result = ERROR_ACCESS_DENIED;
    if (ReadEnabled) {
        result = OriginalRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Registry::getRegReadDebugString(
                hKey,
                lpValueName,
                lpType,
                lpData,
                lpcbData,
                result,
                true
            )
        );
    }
    return result;
}

LONG WINAPI HookMethods::Registry::Write::RegSetValueExAHook(
    HKEY        hKey,
    LPCSTR      lpValueName,
    DWORD       Reserved,
    DWORD       dwType,
    const BYTE* lpData,
    DWORD       cbData
)
{
    LONG result = ERROR_ACCESS_DENIED;
    if (WriteEnabled) {
        result = OriginalRegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Registry::getRegWriteDebugString(
                hKey,
                lpValueName,
                dwType,
                lpData,
                cbData,
                result,
                false
            )
        );
    }
    return result;
}

LONG WINAPI HookMethods::Registry::Write::RegSetValueExWHook(
    HKEY        hKey,
    LPCWSTR     lpValueName,
    DWORD       Reserved,
    DWORD       dwType,
    const BYTE* lpData,
    DWORD       cbData
)
{
    LONG result = ERROR_ACCESS_DENIED;
    if (WriteEnabled) {
        result = OriginalRegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::Registry::getRegWriteDebugString(
                hKey,
                lpValueName,
                dwType,
                lpData,
                cbData,
                result,
                true
            )
        );
    }
    return result;
}

static std::string DecodeAllocType(DWORD type)
{
    std::string out;
    if (type & MEM_COMMIT)      out += "COMMIT|";
    if (type & MEM_RESERVE)     out += "RESERVE|";
    if (type & MEM_RESET)       out += "RESET|";
    if (type & MEM_RESET_UNDO)  out += "RESET_UNDO|";
    if (type & MEM_LARGE_PAGES) out += "LARGE_PAGES|";
    if (type & MEM_PHYSICAL)    out += "PHYSICAL|";
    if (type & MEM_TOP_DOWN)    out += "TOP_DOWN|";
    if (!out.empty()) out.pop_back();
    return out.empty() ? "NONE" : out;
}

static std::string DecodeFreeType(DWORD type)
{
    if (type & MEM_RELEASE)   return "RELEASE";
    if (type & MEM_DECOMMIT)  return "DECOMMIT";
    if (type & MEM_COALESCE_PLACEHOLDERS) return "COALESCE_PLACEHOLDERS";
    if (type & MEM_PRESERVE_PLACEHOLDER)  return "PRESERVE_PLACEHOLDER";
    return "UNKNOWN(" + std::to_string(type) + ")";
}

static std::string DecodeProtect(DWORD protect)
{
    std::string base;
    switch (protect & 0xFF) {
    case PAGE_NOACCESS:          base = "NOACCESS";          break;
    case PAGE_READONLY:          base = "READONLY";          break;
    case PAGE_READWRITE:         base = "READWRITE";         break;
    case PAGE_WRITECOPY:         base = "WRITECOPY";         break;
    case PAGE_EXECUTE:           base = "EXECUTE";           break;
    case PAGE_EXECUTE_READ:      base = "EXECUTE_READ";      break;
    case PAGE_EXECUTE_READWRITE: base = "EXECUTE_READWRITE"; break;
    case PAGE_EXECUTE_WRITECOPY: base = "EXECUTE_WRITECOPY"; break;
    default:                     base = "UNKNOWN(" + std::to_string(protect & 0xFF) + ")"; break;
    }
    if (protect & PAGE_GUARD)        base += "|GUARD";
    if (protect & PAGE_NOCACHE)      base += "|NOCACHE";
    if (protect & PAGE_WRITECOMBINE) base += "|WRITECOMBINE";
    return base;
}

static std::string DecodeHeapFlags(DWORD flags)
{
    if (flags == 0) return "NONE";
    std::string out;
    if (flags & HEAP_NO_SERIALIZE)              out += "NO_SERIALIZE|";
    if (flags & HEAP_ZERO_MEMORY)               out += "ZERO_MEMORY|";
    if (flags & HEAP_GENERATE_EXCEPTIONS)       out += "GENERATE_EXCEPTIONS|";
    if (flags & HEAP_REALLOC_IN_PLACE_ONLY)     out += "IN_PLACE_ONLY|";
    if (!out.empty()) out.pop_back();
    return out.empty() ? "NONE" : out;
}

LPVOID WINAPI HookMethods::Memory::Alloc::VirtualAllocHook(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flAllocationType,
    DWORD  flProtect
)
{
    LPVOID result = nullptr;
    //if (AllocEnabled) {
        result = originalVirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
    //}
    if (DebugEnabled) {
        std::ostringstream ss;
        ss << GetTrackStr("MEMORY ALLOC")
            << " | " << (AllocEnabled ? "ALLOWED" : "BLOCKED")
            << "\n    Time    : " << Utility::GetTimestamp()
            << "\n    API     : VirtualAlloc"
            << "\n    Address : " << lpAddress
            << "\n    Size    : " << dwSize
            << "\n    Type    : " << DecodeAllocType(flAllocationType)
            << "\n    Protect : " << DecodeProtect(flProtect)
            << "\n    Result  : " << result;
        messenger::PutMessage(ss.str());
    }
    return result;
}

LPVOID WINAPI HookMethods::Memory::Alloc::VirtualAllocExHook(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flAllocationType,
    DWORD  flProtect
)
{
    LPVOID result = nullptr;
    //if (AllocEnabled) {
        result = originalVirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
    //}
    if (DebugEnabled) {
        std::ostringstream ss;
        ss << GetTrackStr("MEMORY ALLOC EX")
            << " | " << (AllocEnabled ? "ALLOWED" : "BLOCKED")
            << "\n    Time    : " << Utility::GetTimestamp()
            << "\n    API     : VirtualAllocEx"
            << "\n    Process : " << hProcess
            << "\n    Address : " << lpAddress
            << "\n    Size    : " << dwSize
            << "\n    Type    : " << DecodeAllocType(flAllocationType)
            << "\n    Protect : " << DecodeProtect(flProtect)
            << "\n    Result  : " << result;
        messenger::PutMessage(ss.str());
    }
    return result;
}

LPVOID WINAPI HookMethods::Memory::Alloc::HeapAllocHook(
    HANDLE hHeap,
    DWORD  dwFlags,
    SIZE_T dwBytes
)
{
    LPVOID result = nullptr;
    //if (AllocEnabled) {
        result = originalHeapAlloc(hHeap, dwFlags, dwBytes);
    //}
    if (DebugEnabled) {
        std::ostringstream ss;
        ss << GetTrackStr("HEAP ALLOC")
            << " | " << (AllocEnabled ? "ALLOWED" : "BLOCKED")
            << "\n    Time    : " << Utility::GetTimestamp()
            << "\n    API     : HeapAlloc"
            << "\n    Heap    : " << hHeap
            << "\n    Flags   : " << DecodeHeapFlags(dwFlags)
            << "\n    Size    : " << dwBytes
            << "\n    Result  : " << result;
        messenger::PutMessage(ss.str());
    }
    return result;
}

LPVOID WINAPI HookMethods::Memory::Alloc::HeapReAllocHook(
    HANDLE hHeap,
    DWORD  dwFlags,
    LPVOID lpMem,
    SIZE_T dwBytes
)
{
    LPVOID result = nullptr;
    //if (AllocEnabled) {
        result = originalHeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
    //}
    if (DebugEnabled) {
        std::ostringstream ss;
        ss << GetTrackStr("HEAP REALLOC")
            << " | " << (AllocEnabled ? "ALLOWED" : "BLOCKED")
            << "\n    Time    : " << Utility::GetTimestamp()
            << "\n    API     : HeapReAlloc"
            << "\n    Heap    : " << hHeap
            << "\n    Flags   : " << DecodeHeapFlags(dwFlags)
            << "\n    OldPtr  : " << lpMem
            << "\n    Size    : " << dwBytes
            << "\n    Result  : " << result;
        messenger::PutMessage(ss.str());
    }
    return result;
}

BOOL WINAPI HookMethods::Memory::Free::VirtualFreeHook(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  dwFreeType
)
{
    BOOL result = FALSE;
    //if (FreeEnabled) {
        result = originalVirtualFree(lpAddress, dwSize, dwFreeType);
    //}
    if (DebugEnabled) {
        std::ostringstream ss;
        ss << GetTrackStr("MEMORY FREE")
            << " | " << (FreeEnabled ? "ALLOWED" : "BLOCKED")
            << "\n    Time    : " << Utility::GetTimestamp()
            << "\n    API     : VirtualFree"
            << "\n    Address : " << lpAddress
            << "\n    Size    : " << dwSize
            << "\n    Type    : " << DecodeFreeType(dwFreeType)
            << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
        messenger::PutMessage(ss.str());
    }
    return result;
}

BOOL WINAPI HookMethods::Memory::Free::VirtualFreeExHook(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  dwFreeType
)
{
    BOOL result = FALSE;
    //if (FreeEnabled) {
        result = originalVirtualFreeEx(hProcess, lpAddress, dwSize, dwFreeType);
    //}
    if (DebugEnabled) {
        std::ostringstream ss;
        ss << GetTrackStr("MEMORY FREE EX")
            << " | " << (FreeEnabled ? "ALLOWED" : "BLOCKED")
            << "\n    Time    : " << Utility::GetTimestamp()
            << "\n    API     : VirtualFreeEx"
            << "\n    Process : " << hProcess
            << "\n    Address : " << lpAddress
            << "\n    Size    : " << dwSize
            << "\n    Type    : " << DecodeFreeType(dwFreeType)
            << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
        messenger::PutMessage(ss.str());
    }
    return result;
}

BOOL WINAPI HookMethods::Memory::Free::HeapFreeHook(
    HANDLE hHeap,
    DWORD  dwFlags,
    LPVOID lpMem
)
{
    BOOL result = FALSE;
    //if (FreeEnabled) {
        result = originalHeapFree(hHeap, dwFlags, lpMem);
    //}
    if (DebugEnabled) {
        std::ostringstream ss;
        ss << GetTrackStr("HEAP FREE")
            << " | " << (FreeEnabled ? "ALLOWED" : "BLOCKED")
            << "\n    Time    : " << Utility::GetTimestamp()
            << "\n    API     : HeapFree"
            << "\n    Heap    : " << hHeap
            << "\n    Flags   : " << DecodeHeapFlags(dwFlags)
            << "\n    Ptr     : " << lpMem
            << "\n    Result  : " << (result ? "SUCCESS" : "FAILED");
        messenger::PutMessage(ss.str());
    }
    return result;
}
