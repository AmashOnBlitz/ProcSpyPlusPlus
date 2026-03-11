#pragma once
#include "Windows.h"
#include <string>

#define STR_TRACKPREFIX "[Activity] -> "

inline std::string GetTrackStr(std::string str) {
    return std::string(STR_TRACKPREFIX + str);
}

namespace HookMethods {
    namespace File {
        namespace Write {
            inline std::atomic<bool> WriteEnabled = true;
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

    namespace Utility {
        std::string DecodeAccess(DWORD access);
        std::string DecodeFlags(DWORD flags);
        std::string WStringToString(LPCWSTR wstr);
        std::string DecodeCreation(DWORD creation);
        std::string DecodeShareMode(DWORD share);
        std::string GetTimestamp();

        namespace File {
            std::string getFileWriteDebugString(
                const void* fileName,
                DWORD access,
                DWORD shareMode,
                LPSECURITY_ATTRIBUTES secAttr,
                DWORD creation,
                DWORD flags,
                HANDLE templateFile,
                bool isWide
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
        }// !MsgBox
    }// !Utility
}