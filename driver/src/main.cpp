#include "driver.cpp"

extern "C" {
    NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName,
                                        PDRIVER_INITIALIZE InitializationFunction);

    NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress,
                                             PEPROCESS TargetProcess, PVOID TargetAddress,
                                             SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode,
                                             PSIZE_T ReturnSize);
}

static void debug_print(PCSTR text) {
#ifndef DEBUG
    UNREFERENCED_PARAMETER(text);
#endif

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, text));
}

// Our "real" entry point.
static NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
    UNREFERENCED_PARAMETER(registry_path);

    UNICODE_STRING device_name = {};
    RtlInitUnicodeString(&device_name, L"\\Device\\asumpte");

    // Create driver device obj.
    PDEVICE_OBJECT device_object = nullptr;
    NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN,
                                     FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
    if (status != STATUS_SUCCESS) {
        debug_print("[-] Failed to create driver device.\n");
        return status;
    }

    debug_print("[+] Driver device successfully created.\n");

    UNICODE_STRING symbolic_link = {};
    RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\asumpte");

    status = IoCreateSymbolicLink(&symbolic_link, &device_name);
    if (status != STATUS_SUCCESS) {
        debug_print("[-] Failed to establish symbolic link.\n");
        return status;
    }

    debug_print("[+] Driver symbolic link successfully established.\n");

    // Allow us to send small amounts of data between um/km.
    SetFlag(device_object->Flags, DO_BUFFERED_IO);

    // Set the driver handlers to our functions with our logic.
    driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
    driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
    driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

    // We have initialized our device.
    ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

    debug_print("[+] Driver initialized successfully.\n");

    return status;
}

// KdMapper will call this "entry point" but params will be null.
NTSTATUS DriverEntry() {
    debug_print("[+] Driver loaded successfully.\n");

    UNICODE_STRING driver_name = {};
    RtlInitUnicodeString(&driver_name, L"\\Driver\\asumpte");

    return IoCreateDriver(&driver_name, &driver_main);
}
