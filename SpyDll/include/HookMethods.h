#pragma once
#include "Windows.h"
#include <string>

#define STR_TRACKPREFIX "[Activity] -> "

inline std::string GetTrackStr(std::string str) {
    return std::string(STR_TRACKPREFIX + str);
}

namespace HookMethods {
    namespace File {
        namespace Creation {
            inline std::atomic<bool> CreateEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;
            typedef HANDLE(WINAPI* CreateFileA_t)(
                LPCSTR,
                DWORD,
                DWORD,
                LPSECURITY_ATTRIBUTES,
                DWORD,
                DWORD,
                HANDLE
                );
            typedef HANDLE(WINAPI* CreateFileW_t)(
                LPCWSTR,
                DWORD,
                DWORD,
                LPSECURITY_ATTRIBUTES,
                DWORD,
                DWORD,
                HANDLE
                );
            inline CreateFileA_t OriginalCreateFileA = nullptr;
            inline CreateFileW_t OriginalCreateFileW = nullptr;

            HANDLE WINAPI CreateFileWHook(
                LPCWSTR fileName,
                DWORD access,
                DWORD shareMode,
                LPSECURITY_ATTRIBUTES secAttr,
                DWORD creation,
                DWORD flags,
                HANDLE templateFile
            );
            HANDLE WINAPI CreateFileAHook(
                LPCSTR fileName,
                DWORD access,
                DWORD shareMode,
                LPSECURITY_ATTRIBUTES secAttr,
                DWORD creation,
                DWORD flags,
                HANDLE templateFile
            );
        } // !Creation
        namespace Read {
            inline std::atomic<bool> ReadEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef BOOL(WINAPI* ReadFile_t)(
                HANDLE,
                LPVOID,
                DWORD,
                LPDWORD,
                LPOVERLAPPED
                );
            typedef BOOL(WINAPI* ReadFileEx_t)(
                HANDLE,
                LPVOID,
                DWORD,
                LPOVERLAPPED,
                LPOVERLAPPED_COMPLETION_ROUTINE
                );

            inline ReadFile_t   OriginalReadFile = nullptr;
            inline ReadFileEx_t OriginalReadFileEx = nullptr;

            BOOL WINAPI ReadFileHook(
                HANDLE hFile,
                LPVOID lpBuffer,
                DWORD nNumberOfBytesToRead,
                LPDWORD lpNumberOfBytesRead,
                LPOVERLAPPED lpOverlapped
            );
            BOOL WINAPI ReadFileExHook(
                HANDLE hFile,
                LPVOID lpBuffer,
                DWORD nNumberOfBytesToRead,
                LPOVERLAPPED lpOverlapped,
                LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
            );
        } // !Read
        namespace Write {
            inline std::atomic<bool> WriteEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef BOOL(WINAPI* WriteFile_t)(
                HANDLE,
                LPCVOID,
                DWORD,
                LPDWORD,
                LPOVERLAPPED
                );
            typedef BOOL(WINAPI* WriteFileEx_t)(
                HANDLE,
                LPCVOID,
                DWORD,
                LPOVERLAPPED,
                LPOVERLAPPED_COMPLETION_ROUTINE
                );

            inline WriteFile_t   OriginalWriteFile = nullptr;
            inline WriteFileEx_t OriginalWriteFileEx = nullptr;

            BOOL WINAPI WriteFileHook(
                HANDLE       hFile,
                LPCVOID      lpBuffer,
                DWORD        nNumberOfBytesToWrite,
                LPDWORD      lpNumberOfBytesWritten,
                LPOVERLAPPED lpOverlapped
            );
            BOOL WINAPI WriteFileExHook(
                HANDLE       hFile,
                LPCVOID      lpBuffer,
                DWORD        nNumberOfBytesToWrite,
                LPOVERLAPPED lpOverlapped,
                LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
            );
        } // !Write
    } // !File

    namespace MsgBox {
        inline std::atomic<bool> MsgBoxEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        typedef int (WINAPI* MessageBoxA_t)(
            HWND,
            LPCSTR,
            LPCSTR,
            UINT
            );
        typedef int (WINAPI* MessageBoxW_t)(
            HWND,
            LPCWSTR,
            LPCWSTR,
            UINT
            );

        inline MessageBoxA_t OriginalMessageBoxA = nullptr;
        inline MessageBoxW_t OriginalMessageBoxW = nullptr;

        int WINAPI MessageBoxAHook(
            HWND hWnd,
            LPCSTR text,
            LPCSTR caption,
            UINT type
        );
        int WINAPI MessageBoxWHook(
            HWND hWnd,
            LPCWSTR text,
            LPCWSTR caption,
            UINT type
        );
    }; // !MsgBox

    namespace Registry {
        namespace Read {
            inline std::atomic<bool> ReadEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef LONG(WINAPI* RegQueryValueExA_t)(
                HKEY,
                LPCSTR,
                LPDWORD,
                LPDWORD,
                LPBYTE,
                LPDWORD
                );
            typedef LONG(WINAPI* RegQueryValueExW_t)(
                HKEY,
                LPCWSTR,
                LPDWORD,
                LPDWORD,
                LPBYTE,
                LPDWORD
                );

            inline RegQueryValueExA_t OriginalRegQueryValueExA = nullptr;
            inline RegQueryValueExW_t OriginalRegQueryValueExW = nullptr;

            LONG WINAPI RegQueryValueExAHook(
                HKEY    hKey,
                LPCSTR  lpValueName,
                LPDWORD lpReserved,
                LPDWORD lpType,
                LPBYTE  lpData,
                LPDWORD lpcbData
            );
            LONG WINAPI RegQueryValueExWHook(
                HKEY    hKey,
                LPCWSTR lpValueName,
                LPDWORD lpReserved,
                LPDWORD lpType,
                LPBYTE  lpData,
                LPDWORD lpcbData
            );
        } // !Read

        namespace Write {
            inline std::atomic<bool> WriteEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef LONG(WINAPI* RegSetValueExA_t)(
                HKEY,
                LPCSTR,
                DWORD,
                DWORD,
                const BYTE*,
                DWORD
                );
            typedef LONG(WINAPI* RegSetValueExW_t)(
                HKEY,
                LPCWSTR,
                DWORD,
                DWORD,
                const BYTE*,
                DWORD
                );

            inline RegSetValueExA_t OriginalRegSetValueExA = nullptr;
            inline RegSetValueExW_t OriginalRegSetValueExW = nullptr;

            LONG WINAPI RegSetValueExAHook(
                HKEY        hKey,
                LPCSTR      lpValueName,
                DWORD       Reserved,
                DWORD       dwType,
                const BYTE* lpData,
                DWORD       cbData
            );
            LONG WINAPI RegSetValueExWHook(
                HKEY        hKey,
                LPCWSTR     lpValueName,
                DWORD       Reserved,
                DWORD       dwType,
                const BYTE* lpData,
                DWORD       cbData
            );
        } // !Write
    } // !Registry

    namespace Utility {
        std::string DecodeAccess(DWORD access);
        std::string DecodeFlags(DWORD flags);
        std::string WStringToString(LPCWSTR wstr);
        std::string DecodeCreation(DWORD creation);
        std::string DecodeShareMode(DWORD share);
        std::string DecodeFileIntent(DWORD access, DWORD creation, DWORD flags, bool fileExisted);
        std::string GetTimestamp();

        namespace File {
            std::string GetPathFromHandle(HANDLE hFile);

            std::string getFileCreateDebugString(
                const void* fileName,
                DWORD access,
                DWORD shareMode,
                LPSECURITY_ATTRIBUTES secAttr,
                DWORD creation,
                DWORD flags,
                HANDLE templateFile,
                bool isWide
            );

            std::string getFileReadDebugString(
                HANDLE hFile,
                LPVOID buffer,
                DWORD bytesToRead,
                LPDWORD bytesRead,
                LPOVERLAPPED overlapped
            );

            std::string getFileReadExDebugString(
                HANDLE hFile,
                LPVOID lpBuffer,
                DWORD nNumberOfBytesToRead,
                LPOVERLAPPED lpOverlapped,
                LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
            );

            std::string getFileWriteDebugString(
                HANDLE hFile,
                LPCVOID buffer,
                DWORD bytesToWrite,
                LPDWORD bytesWritten,
                LPOVERLAPPED overlapped,
                bool allowed
            );

            std::string getFileWriteExDebugString(
                HANDLE hFile,
                LPCVOID lpBuffer,
                DWORD nNumberOfBytesToWrite,
                LPOVERLAPPED lpOverlapped,
                LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
            );
        }; // !File

        namespace MsgBox {
            std::string getMsgBoxDebugString(
                HWND hWnd,
                const void* text,
                const void* caption,
                UINT type,
                bool isWide
            );
        } // !MsgBox

        namespace Registry {
            std::string GetKeyNameFromHandle(HKEY hKey);
            std::string DecodeRegType(DWORD type);

            std::string getRegReadDebugString(
                HKEY hKey,
                const void* lpValueName,
                LPDWORD lpType,
                LPBYTE lpData,
                LPDWORD lpcbData,
                LONG result,
                bool isWide
            );

            std::string getRegWriteDebugString(
                HKEY hKey,
                const void* lpValueName,
                DWORD dwType,
                const BYTE* lpData,
                DWORD cbData,
                LONG result,
                bool isWide
            );
        } // !Registry
    } // !Utility
}
