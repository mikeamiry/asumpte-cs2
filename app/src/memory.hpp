#pragma once

#include <iostream>

#include <Windows.h>
#include <TlHelp32.h>

namespace memory {

    static DWORD get_process_id(const wchar_t* process_name);
    static std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name);
    static HWND get_window_handle_from_process_id(const DWORD ProcessId);

    static DWORD memory::get_process_id(const wchar_t* process_name) {
        DWORD process_id = 0;

        HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (snap_shot == INVALID_HANDLE_VALUE)
            return process_id;

        PROCESSENTRY32W entry = {};
        entry.dwSize = sizeof(decltype(entry));

        if (Process32FirstW(snap_shot, &entry) == TRUE) {
            // Check if the first handle is the one we want.
            if (_wcsicmp(process_name, entry.szExeFile) == 0)
                process_id = entry.th32ProcessID;
            else {
                while (Process32NextW(snap_shot, &entry) == TRUE) {
                    if (_wcsicmp(process_name, entry.szExeFile) == 0) {
                        process_id = entry.th32ProcessID;
                        break;
                    }
                }
            }
        }

        CloseHandle(snap_shot);

        return process_id;
    }

    static std::uintptr_t memory::get_module_base(const DWORD pid, const wchar_t* module_name) {
        std::uintptr_t module_base = 0;

        // Snap-shot of process' modules (dlls).
        HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap_shot == INVALID_HANDLE_VALUE)
            return module_base;

        MODULEENTRY32W entry = {};
        entry.dwSize = sizeof(decltype(entry));

        if (Module32FirstW(snap_shot, &entry) == TRUE) {
            if (wcsstr(module_name, entry.szModule) != nullptr)
                module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
            else {
                while (Module32NextW(snap_shot, &entry) == TRUE) {
                    if (wcsstr(module_name, entry.szModule) != nullptr) {
                        module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
                        break;
                    }
                }
            }
        }

        CloseHandle(snap_shot);

        return module_base;
    }

    HWND memory::get_window_handle_from_process_id(const DWORD ProcessId) {

        HWND hwnd = nullptr;
        do {
            hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr);
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == ProcessId) {
                TCHAR windowTitle[MAX_PATH];
                GetWindowText(hwnd, windowTitle, MAX_PATH);
                if (IsWindowVisible(hwnd) && windowTitle[0] != '\0') {
                    return hwnd;
                }
            }
        } while (hwnd != nullptr);

        return nullptr; // No main window found for the given process ID
    }
}