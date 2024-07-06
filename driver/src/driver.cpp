#include <ntifs.h>

namespace driver {
    namespace codes {
        // Used to setup the driver.
        constexpr ULONG attach =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

        // Read process memory.
        constexpr ULONG read =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

        // Read process memory.
        constexpr ULONG write =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    }  // namespace codes

    // Shared between user mode & kernel mode.
    struct Request {
        HANDLE process_id;

        PVOID target;
        PVOID buffer;

        SIZE_T size;
        SIZE_T return_size;
    };

    NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);

        IoCompleteRequest(irp, IO_NO_INCREMENT);

        return irp->IoStatus.Status;
    }

    NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);

        IoCompleteRequest(irp, IO_NO_INCREMENT);

        return irp->IoStatus.Status;
    }

    NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);

        debug_print("[asumpte] Device control called.\n");

        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // We need this to determine which code was passed through.
        PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

        // Access the request object sent from user mode.
        auto request = reinterpret_cast<Request*>(irp->AssociatedIrp.SystemBuffer);

        if (stack_irp == nullptr || request == nullptr) {
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return status;
        }

        // The target process we want access to.
        static PEPROCESS target_process = nullptr;

        const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;
        switch (control_code) {
        case codes::attach:
            status = PsLookupProcessByProcessId(request->process_id, &target_process);
            break;

            // Read process memory implementation.
        case codes::read:
            if (target_process != nullptr)
                status = MmCopyVirtualMemory(target_process, request->target,
                    PsGetCurrentProcess(), request->buffer,
                    request->size, KernelMode, &request->return_size);
            break;

        case codes::write:
            if (target_process != nullptr)
                status = MmCopyVirtualMemory(PsGetCurrentProcess(), request->buffer,
                    target_process, request->target, request->size,
                    KernelMode, &request->return_size);
            break;

        default:
            break;
        }

        irp->IoStatus.Status = status;
        irp->IoStatus.Information = sizeof(Request);

        IoCompleteRequest(irp, IO_NO_INCREMENT);

        return status;
    }

}  // namespace driver