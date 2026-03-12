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
