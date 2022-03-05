/*
  Copyright (c) 2022  Kazuhiro Ito (kzhr@d1.dion.ne.jp)

  Original file is automatically generated file by Visual Studio 2019

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/


//
// Define below GUIDs
//
#include <initguid.h>
#include <winusb.h>

//
// Device Interface GUID.
// Used by all WinUsb devices that this application talks to.
// Must match "DeviceInterfaceGUIDs" registry value specified in the INF file.
// 4945d185-c9ab-4caa-8976-66659bfae649
//
DEFINE_GUID(GUID_DEVINTERFACE_emi26flw,
    0x4945d185,0xc9ab,0x4caa,0x89,0x76,0x66,0x65,0x9b,0xfa,0xe6,0x49);

typedef struct _DEVICE_DATA {

    BOOL                    HandlesOpen;
    WINUSB_INTERFACE_HANDLE WinusbHandle;
    HANDLE                  DeviceHandle;
    TCHAR                   DevicePath[MAX_PATH];

} DEVICE_DATA, *PDEVICE_DATA;

HRESULT
OpenDevice(
    _Out_     PDEVICE_DATA DeviceData,
    _Out_opt_ PBOOL        FailureDeviceNotFound
    );

VOID
CloseDevice(
    _Inout_ PDEVICE_DATA DeviceData
    );
