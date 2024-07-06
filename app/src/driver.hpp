#pragma once

#include <iostream>
#include <Windows.h>

namespace driver {
    namespace codes {
        // Used to setup the driver.
        constexpr ULONG attach =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

        // Read process memory.
        constexpr ULONG read =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

    }  // namespace codes

    // Shared between user mode & kernel mode.
    struct Request {
        HANDLE process_id;

        PVOID target;
        PVOID buffer;

        SIZE_T size;
        SIZE_T return_size;
    };

    bool attach_to_process(HANDLE driver_handle, const DWORD pid) {
        Request r;
        r.process_id = reinterpret_cast<HANDLE>(pid);

        return DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr,
            nullptr);
    }

    template <class T>
    T read_memory(HANDLE driver_handle, const std::uintptr_t addr) {
        T temp{};

        Request r;
        r.target = reinterpret_cast<PVOID>(addr);
        r.buffer = &temp;
        r.size = sizeof(T);

        DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

        return temp;
    }

}