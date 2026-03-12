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

    namespace Dialog {
        inline std::atomic<bool> DialogEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        typedef INT_PTR(WINAPI* DialogBoxParamA_t)(
            HINSTANCE,
            LPCSTR,
            HWND,
            DLGPROC,
            LPARAM
            );
        typedef INT_PTR(WINAPI* DialogBoxParamW_t)(
            HINSTANCE,
            LPCWSTR,
            HWND,
            DLGPROC,
            LPARAM
            );
        typedef HWND(WINAPI* CreateDialogParamA_t)(
            HINSTANCE,
            LPCSTR,
            HWND,
            DLGPROC,
            LPARAM
            );
        typedef HWND(WINAPI* CreateDialogParamW_t)(
            HINSTANCE,
            LPCWSTR,
            HWND,
            DLGPROC,
            LPARAM
            );
        typedef HRESULT(WINAPI* TaskDialog_t)(
            HWND,
            HINSTANCE,
            PCWSTR,
            PCWSTR,
            PCWSTR,
            TASKDIALOG_COMMON_BUTTON_FLAGS,
            PCWSTR,
            int*
            );
        typedef HRESULT(WINAPI* TaskDialogIndirect_t)(
            const TASKDIALOGCONFIG*,
            int*,
            int*,
            BOOL*
            );

        inline DialogBoxParamA_t    OriginalDialogBoxParamA    = nullptr;
        inline DialogBoxParamW_t    OriginalDialogBoxParamW    = nullptr;
        inline CreateDialogParamA_t OriginalCreateDialogParamA = nullptr;
        inline CreateDialogParamW_t OriginalCreateDialogParamW = nullptr;
        inline TaskDialog_t         OriginalTaskDialog         = nullptr;
        inline TaskDialogIndirect_t OriginalTaskDialogIndirect = nullptr;

        INT_PTR WINAPI DialogBoxParamAHook(
            HINSTANCE hInstance,
            LPCSTR    lpTemplateName,
            HWND      hWndParent,
            DLGPROC   lpDialogFunc,
            LPARAM    dwInitParam
        );
        INT_PTR WINAPI DialogBoxParamWHook(
            HINSTANCE hInstance,
            LPCWSTR   lpTemplateName,
            HWND      hWndParent,
            DLGPROC   lpDialogFunc,
            LPARAM    dwInitParam
        );
        HWND WINAPI CreateDialogParamAHook(
            HINSTANCE hInstance,
            LPCSTR    lpTemplateName,
            HWND      hWndParent,
            DLGPROC   lpDialogFunc,
            LPARAM    dwInitParam
        );
        HWND WINAPI CreateDialogParamWHook(
            HINSTANCE hInstance,
            LPCWSTR   lpTemplateName,
            HWND      hWndParent,
            DLGPROC   lpDialogFunc,
            LPARAM    dwInitParam
        );
        HRESULT WINAPI TaskDialogHook(
            HWND                           hwndOwner,
            HINSTANCE                      hInstance,
            PCWSTR                         pszWindowTitle,
            PCWSTR                         pszMainInstruction,
            PCWSTR                         pszContent,
            TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
            PCWSTR                         pszIcon,
            int*                           pnButton
        );
        HRESULT WINAPI TaskDialogIndirectHook(
            const TASKDIALOGCONFIG* pTaskConfig,
            int*                    pnButton,
            int*                    pnRadioButton,
            BOOL*                   pfVerificationFlagChecked
        );
    }; // !Dialog

    namespace Window {
        inline std::atomic<bool> WindowEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        // CreateWindowA/W are macros that expand to CreateWindowExA/W with dwExStyle=0,
        // so hooking the Ex variants covers all four call paths.
        typedef HWND(WINAPI* CreateWindowExA_t)(
            DWORD,
            LPCSTR,
            LPCSTR,
            DWORD,
            int,
            int,
            int,
            int,
            HWND,
            HMENU,
            HINSTANCE,
            LPVOID
            );
        typedef HWND(WINAPI* CreateWindowExW_t)(
            DWORD,
            LPCWSTR,
            LPCWSTR,
            DWORD,
            int,
            int,
            int,
            int,
            HWND,
            HMENU,
            HINSTANCE,
            LPVOID
            );

        inline CreateWindowExA_t OriginalCreateWindowExA = nullptr;
        inline CreateWindowExW_t OriginalCreateWindowExW = nullptr;

        HWND WINAPI CreateWindowExAHook(
            DWORD     dwExStyle,
            LPCSTR    lpClassName,
            LPCSTR    lpWindowName,
            DWORD     dwStyle,
            int       X,
            int       Y,
            int       nWidth,
            int       nHeight,
            HWND      hWndParent,
            HMENU     hMenu,
            HINSTANCE hInstance,
            LPVOID    lpParam
        );
        HWND WINAPI CreateWindowExWHook(
            DWORD     dwExStyle,
            LPCWSTR   lpClassName,
            LPCWSTR   lpWindowName,
            DWORD     dwStyle,
            int       X,
            int       Y,
            int       nWidth,
            int       nHeight,
            HWND      hWndParent,
            HMENU     hMenu,
            HINSTANCE hInstance,
            LPVOID    lpParam
        );
    }; // !Window

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

        namespace Dialog {
            std::string getDialogBoxParamDebugString(
                HINSTANCE   hInstance,
                const void* lpTemplateName,
                HWND        hWndParent,
                DLGPROC     lpDialogFunc,
                LPARAM      dwInitParam,
                bool        isWide
            );
            std::string getCreateDialogParamDebugString(
                HINSTANCE   hInstance,
                const void* lpTemplateName,
                HWND        hWndParent,
                DLGPROC     lpDialogFunc,
                LPARAM      dwInitParam,
                bool        isWide
            );
            std::string getTaskDialogDebugString(
                HWND                           hwndOwner,
                PCWSTR                         pszWindowTitle,
                PCWSTR                         pszMainInstruction,
                PCWSTR                         pszContent,
                TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
                PCWSTR                         pszIcon
            );
            std::string getTaskDialogIndirectDebugString(
                const TASKDIALOGCONFIG* pTaskConfig
            );
        } // !Dialog

        namespace Window {
            std::string DecodeWindowStyle(DWORD dwStyle);
            std::string DecodeWindowExStyle(DWORD dwExStyle);

            std::string getCreateWindowExDebugString(
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
            );
        } // !Window

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