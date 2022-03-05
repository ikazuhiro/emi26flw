#include <ntdef.h>
#include <winusb.h>

NTSTATUS emi26_load_files(const wchar_t* directory);
NTSTATUS emi26_load_firmware(WINUSB_INTERFACE_HANDLE interfaceHandle, const wchar_t* directory);
