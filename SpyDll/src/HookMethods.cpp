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
    LPVOID buffer,
    DWORD bytesToRead,
    LPDWORD bytesRead,
    LPOVERLAPPED overlapped
)
{
    if (GetFileType(hFile) == FILE_TYPE_PIPE) {
        return OriginalReadFile(hFile, buffer, bytesToRead, bytesRead, overlapped);
    }

    BOOL result = FALSE;
    if (ReadEnabled) {
        result = OriginalReadFile(hFile, buffer, bytesToRead, bytesRead, overlapped);
    }
    if (DebugEnabled) {
        messenger::PutMessage(
            Utility::File::getFileReadDebugString(
                hFile,
                buffer,
                bytesToRead,
                bytesRead,
                overlapped
            )
        );
    }
    return result;
}

BOOL WINAPI HookMethods::File::Write::WriteFileHook(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
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
                lpOverlapped
            )
        );
    }

    return result;
}