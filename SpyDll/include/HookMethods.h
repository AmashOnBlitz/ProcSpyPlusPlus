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

        inline DialogBoxParamA_t    OriginalDialogBoxParamA = nullptr;
        inline DialogBoxParamW_t    OriginalDialogBoxParamW = nullptr;
        inline CreateDialogParamA_t OriginalCreateDialogParamA = nullptr;
        inline CreateDialogParamW_t OriginalCreateDialogParamW = nullptr;
        inline TaskDialog_t         OriginalTaskDialog = nullptr;
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
            int* pnButton
        );
        HRESULT WINAPI TaskDialogIndirectHook(
            const TASKDIALOGCONFIG* pTaskConfig,
            int* pnButton,
            int* pnRadioButton,
            BOOL* pfVerificationFlagChecked
        );
    }; // !Dialog

    // DEPRECATED: memory hooks not used - causes deadlock due to re-entrant heap allocations during hook logging
    namespace Memory {
        namespace Alloc {
            inline std::atomic<bool> AllocEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef LPVOID(WINAPI* VirtualAlloc_t)(
                LPVOID,
                SIZE_T,
                DWORD,
                DWORD
                );
            typedef LPVOID(WINAPI* VirtualAllocEx_t)(
                HANDLE,
                LPVOID,
                SIZE_T,
                DWORD,
                DWORD
                );
            typedef LPVOID(WINAPI* HeapAlloc_t)(
                HANDLE,
                DWORD,
                SIZE_T
                );
            typedef LPVOID(WINAPI* HeapReAlloc_t)(
                HANDLE,
                DWORD,
                LPVOID,
                SIZE_T
                );
            inline VirtualAlloc_t originalVirtualAlloc = nullptr;
            inline VirtualAllocEx_t originalVirtualAllocEx = nullptr;
            inline HeapAlloc_t originalHeapAlloc = nullptr;
            inline HeapReAlloc_t originalHeapReAlloc = nullptr;

            LPVOID WINAPI VirtualAllocHook(
                LPVOID lpAddress,
                SIZE_T dwSize,
                DWORD  flAllocationType,
                DWORD  flProtect
            );
            LPVOID WINAPI VirtualAllocExHook(
                HANDLE hProcess,
                LPVOID lpAddress,
                SIZE_T dwSize,
                DWORD  flAllocationType,
                DWORD  flProtect
            );
            LPVOID WINAPI HeapAllocHook(
                HANDLE hHeap,
                DWORD  dwFlags,
                SIZE_T dwBytes
            );
            LPVOID WINAPI HeapReAllocHook(
                HANDLE hHeap,
                DWORD  dwFlags,
                LPVOID lpMem,
                SIZE_T dwBytes
            );

        }// !Alloc
        namespace Free {
            inline std::atomic<bool> FreeEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef BOOL(WINAPI* VirtualFree_t) (
                LPVOID,
                SIZE_T,
                DWORD
                );

            typedef BOOL(WINAPI* VirtualFreeEx_t)(
                HANDLE,
                LPVOID,
                SIZE_T,
                DWORD
                );
            typedef BOOL(WINAPI* HeapFree_t)(
                HANDLE,
                DWORD,
                LPVOID
                );
            inline VirtualFree_t originalVirtualFree = nullptr;
            inline VirtualFreeEx_t originalVirtualFreeEx = nullptr;
            inline HeapFree_t originalHeapFree = nullptr;

            BOOL WINAPI VirtualFreeHook(
                LPVOID lpAddress,
                SIZE_T dwSize,
                DWORD  dwFreeType
            );
            BOOL WINAPI VirtualFreeExHook(
                HANDLE hProcess,
                LPVOID lpAddress,
                SIZE_T dwSize,
                DWORD  dwFreeType
            );
            BOOL WINAPI HeapFreeHook(
                HANDLE hHeap,
                DWORD  dwFlags,
                LPVOID lpMem
            );
        }// !Free
    } // !Memory

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

    namespace Network {
        namespace Send {
            inline std::atomic<bool> SendEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef int (WINAPI* send_t)(SOCKET, const char*, int, int);
            typedef int (WINAPI* WSASend_t)(
                SOCKET,
                LPWSABUF,
                DWORD,
                LPDWORD,
                DWORD,
                LPWSAOVERLAPPED,
                LPWSAOVERLAPPED_COMPLETION_ROUTINE
                );

            inline send_t    OriginalSend = nullptr;
            inline WSASend_t OriginalWSASend = nullptr;

            int WINAPI sendHook(SOCKET s, const char* buf, int len, int flags);
            int WINAPI WSASendHook(
                SOCKET s,
                LPWSABUF lpBuffers,
                DWORD dwBufferCount,
                LPDWORD lpNumberOfBytesSent,
                DWORD dwFlags,
                LPWSAOVERLAPPED lpOverlapped,
                LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
            );
        } // !Send
        namespace Receive {
            inline std::atomic<bool> ReceiveEnabled = true;
            inline std::atomic<bool> DebugEnabled = false;

            typedef int (WINAPI* recv_t)(SOCKET, char*, int, int);
            typedef int (WINAPI* WSARecv_t)(
                SOCKET,
                LPWSABUF,
                DWORD,
                LPDWORD,
                LPDWORD,
                LPWSAOVERLAPPED,
                LPWSAOVERLAPPED_COMPLETION_ROUTINE
                );

            inline recv_t    OriginalRecv = nullptr;
            inline WSARecv_t OriginalWSARecv = nullptr;

            int WINAPI recvHook(SOCKET s, char* buf, int len, int flags);
            int WINAPI WSARecvHook(
                SOCKET s,
                LPWSABUF lpBuffers,
                DWORD dwBufferCount,
                LPDWORD lpNumberOfBytesRecvd,
                LPDWORD lpFlags,
                LPWSAOVERLAPPED lpOverlapped,
                LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
            );
        } // !Receive
    } // !Network

    namespace Thread {
        inline std::atomic<bool> ThreadEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        typedef HANDLE(WINAPI* CreateThread_t)(
            LPSECURITY_ATTRIBUTES,
            SIZE_T,
            LPTHREAD_START_ROUTINE,
            LPVOID,
            DWORD,
            LPDWORD
            );
        typedef HANDLE(WINAPI* CreateRemoteThreadEx_t)(
            HANDLE,
            LPSECURITY_ATTRIBUTES,
            SIZE_T,
            LPTHREAD_START_ROUTINE,
            LPVOID,
            DWORD,
            LPPROC_THREAD_ATTRIBUTE_LIST,
            LPDWORD
            );

        inline CreateThread_t         OriginalCreateThread = nullptr;
        inline CreateRemoteThreadEx_t OriginalCreateRemoteThreadEx = nullptr;

        HANDLE WINAPI CreateThreadHook(
            LPSECURITY_ATTRIBUTES lpThreadAttributes,
            SIZE_T                dwStackSize,
            LPTHREAD_START_ROUTINE lpStartAddress,
            LPVOID                lpParameter,
            DWORD                 dwCreationFlags,
            LPDWORD               lpThreadId
        );
        HANDLE WINAPI CreateRemoteThreadExHook(
            HANDLE                       hProcess,
            LPSECURITY_ATTRIBUTES        lpThreadAttributes,
            SIZE_T                       dwStackSize,
            LPTHREAD_START_ROUTINE       lpStartAddress,
            LPVOID                       lpParameter,
            DWORD                        dwCreationFlags,
            LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
            LPDWORD                      lpThreadId
        );
    }; // !Thread

    namespace DLL {
        inline std::atomic<bool> DLLEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        typedef HMODULE(WINAPI* LoadLibraryA_t)(LPCSTR);
        typedef HMODULE(WINAPI* LoadLibraryW_t)(LPCWSTR);
        typedef HMODULE(WINAPI* LoadLibraryExA_t)(LPCSTR, HANDLE, DWORD);
        typedef HMODULE(WINAPI* LoadLibraryExW_t)(LPCWSTR, HANDLE, DWORD);

        inline LoadLibraryA_t   OriginalLoadLibraryA = nullptr;
        inline LoadLibraryW_t   OriginalLoadLibraryW = nullptr;
        inline LoadLibraryExA_t OriginalLoadLibraryExA = nullptr;
        inline LoadLibraryExW_t OriginalLoadLibraryExW = nullptr;

        HMODULE WINAPI LoadLibraryAHook(LPCSTR  lpLibFileName);
        HMODULE WINAPI LoadLibraryWHook(LPCWSTR lpLibFileName);
        HMODULE WINAPI LoadLibraryExAHook(LPCSTR  lpLibFileName, HANDLE hFile, DWORD dwFlags);
        HMODULE WINAPI LoadLibraryExWHook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
    }; // !DLL

    namespace Clipboard {
        inline std::atomic<bool> ClipboardEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        typedef BOOL(WINAPI* OpenClipboard_t)(HWND);
        typedef HANDLE(WINAPI* GetClipboardData_t)(UINT);
        typedef HANDLE(WINAPI* SetClipboardData_t)(UINT, HANDLE);
        typedef BOOL(WINAPI* EmptyClipboard_t)();

        inline OpenClipboard_t    OriginalOpenClipboard = nullptr;
        inline GetClipboardData_t OriginalGetClipboardData = nullptr;
        inline SetClipboardData_t OriginalSetClipboardData = nullptr;
        inline EmptyClipboard_t   OriginalEmptyClipboard = nullptr;

        BOOL   WINAPI OpenClipboardHook(HWND hWndNewOwner);
        HANDLE WINAPI GetClipboardDataHook(UINT uFormat);
        HANDLE WINAPI SetClipboardDataHook(UINT uFormat, HANDLE hMem);
        BOOL   WINAPI EmptyClipboardHook();
    }; // !Clipboard

    namespace Screenshot {
        inline std::atomic<bool> ScreenshotEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        typedef BOOL(WINAPI* BitBlt_t)(HDC, int, int, int, int, HDC, int, int, DWORD);

        inline BitBlt_t OriginalBitBlt = nullptr;

        BOOL WINAPI BitBltHook(
            HDC  hdcDest,
            int  x,
            int  y,
            int  nWidth,
            int  nHeight,
            HDC  hdcSrc,
            int  x1,
            int  y1,
            DWORD rop
        );
    }; // !Screenshot

    namespace Window {
        inline std::atomic<bool> WindowEnabled = true;
        inline std::atomic<bool> DebugEnabled = false;

        typedef HWND(WINAPI* CreateWindowExA_t)(
            DWORD, LPCSTR, LPCSTR, DWORD,
            int, int, int, int,
            HWND, HMENU, HINSTANCE, LPVOID
            );
        typedef HWND(WINAPI* CreateWindowExW_t)(
            DWORD, LPCWSTR, LPCWSTR, DWORD,
            int, int, int, int,
            HWND, HMENU, HINSTANCE, LPVOID
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

        namespace Network {
            std::string getNetworkSendDebugString(
                SOCKET s,
                DWORD  bytesSent,
                int    flags,
                int    result,
                bool   allowed
            );
            std::string getNetworkSendExDebugString(
                SOCKET s,
                DWORD  bufferCount,
                DWORD  bytesSent,
                DWORD  wsaFlags,
                bool   async,
                int    result,
                bool   allowed
            );
            std::string getNetworkReceiveDebugString(
                SOCKET s,
                DWORD  bytesRecvd,
                int    flags,
                int    result,
                bool   allowed
            );
            std::string getNetworkReceiveExDebugString(
                SOCKET s,
                DWORD  bufferCount,
                DWORD  bytesRecvd,
                bool   async,
                int    result,
                bool   allowed
            );
        } // !Network

        namespace Thread {
            std::string getCreateThreadDebugString(
                SIZE_T               dwStackSize,
                LPTHREAD_START_ROUTINE lpStartAddress,
                LPVOID               lpParameter,
                DWORD                dwCreationFlags,
                DWORD                threadId,
                HANDLE               result
            );
            std::string getCreateRemoteThreadExDebugString(
                HANDLE               hProcess,
                SIZE_T               dwStackSize,
                LPTHREAD_START_ROUTINE lpStartAddress,
                LPVOID               lpParameter,
                DWORD                dwCreationFlags,
                DWORD                threadId,
                HANDLE               result
            );
        } // !Thread

        namespace DLL {
            std::string getDLLLoadDebugString(
                const void* lpLibFileName,
                DWORD       dwFlags,
                HMODULE     result,
                bool        isWide,
                bool        isEx
            );
        } // !DLL

        namespace Clipboard {
            std::string DecodeClipboardFormat(UINT fmt);
            std::string getOpenClipboardDebugString(HWND hWnd, BOOL result);
            std::string getGetClipboardDataDebugString(UINT uFormat, HANDLE result);
            std::string getSetClipboardDataDebugString(UINT uFormat, HANDLE hMem, HANDLE result);
            std::string getEmptyClipboardDebugString(BOOL result);
        } // !Clipboard

        namespace Screenshot {
            std::string DecodeBitBltRop(DWORD rop);
            std::string getBitBltDebugString(
                int   xDest,
                int   yDest,
                int   nWidth,
                int   nHeight,
                int   xSrc,
                int   ySrc,
                DWORD rop,
                BOOL  result
            );
        } // !Screenshot
    } // !Utility
}