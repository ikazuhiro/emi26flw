/*
  Copyright (c) 2022  Kazuhiro Ito (kzhr@d1.dion.ne.jp)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/


#include <stdio.h>
#include <windows.h>
#include <winreg.h>
#include <strsafe.h>

#include "device.h"

/* main.c */
void errnum_printf_message(PCWSTR string, DWORD errnum);


#define GUID_LENGTH (38)
#define REGKEY_LENGTH (255)

#define EMAGIC_VID (0x086a)
#define EMI26_PID (0x0100)
#define EMI26B_PID (0x0102)

static const WCHAR REGKEY_FORMAT[] = L"SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_%04X&PID_%04X";
static const WCHAR REGKEY_GUID[] = L"DeviceInterfaceGUID";
static const WCHAR SUBKEY_FORMAT[] = L"%s\\Device Parameters";

LSTATUS register_interface_guid(USHORT vid, USHORT pid, PCWSTR guid) {
  HKEY hKey;
  LSTATUS lstatus ;
  WCHAR key[wcslen(REGKEY_FORMAT) + 1];
  StringCchPrintfW(key, wcslen(REGKEY_FORMAT) + 1, REGKEY_FORMAT, vid, pid);

  lstatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WRITE, &hKey);
  if (lstatus != ERROR_SUCCESS) {
    if (lstatus != ERROR_FILE_NOT_FOUND) {
      errnum_printf_message(L"Fail to open registry key", lstatus);
      if (lstatus == ERROR_ACCESS_DENIED)
	wprintf(L"Do you have administrator privileges?\n");
    }
    return lstatus;
  }

  DWORD i = 0;
  WCHAR subkeyName[REGKEY_LENGTH + 1];
  DWORD subkeyNameLength;
  WCHAR subkey2Name[REGKEY_LENGTH + 1];
  DWORD subkey2Length = REGKEY_LENGTH;

  while (1) {
    subkeyNameLength = REGKEY_LENGTH;
    lstatus = RegEnumKeyExW(hKey, i, subkeyName, &subkeyNameLength, NULL, NULL, NULL, NULL);

    if (lstatus == ERROR_SUCCESS) {
      HRESULT hResult = StringCchPrintfW(subkey2Name, subkey2Length + 1, SUBKEY_FORMAT, subkeyName);
      if (FAILED(hResult)) {
	lstatus = ERROR_INSUFFICIENT_BUFFER;
	errnum_printf_message(L"Failed to make subkey name string", lstatus);
	return lstatus;
      }

      lstatus = RegSetKeyValueW(hKey, subkey2Name, REGKEY_GUID, REG_SZ, guid, wcslen(guid) * 2 + 2);
      if (lstatus != ERROR_SUCCESS) {
	errnum_printf_message(L"Failed to write GUID", lstatus);
	return lstatus;
      }
    } else if (lstatus == ERROR_NO_MORE_ITEMS) {
      break;
    } else {
      errnum_printf_message(L"Failed to enumrate subkey", lstatus);
      return lstatus;
    }
    i++;
  }

 finish:
  if (i) wprintf(L"Registered DeviceInterfaceGUID for %d devices.\n", (INT)i);
  RegCloseKey(hKey);
  return ERROR_SUCCESS;
}

LSTATUS emi26r_register_interface_guid() {
  WCHAR guid[GUID_LENGTH + 1];

  if (!StringFromGUID2(&GUID_DEVINTERFACE_emi26flw, guid, GUID_LENGTH + 1)) {
    errnum_printf_message(L"Failed to make GUID string", ERROR_INSUFFICIENT_BUFFER);
    return ERROR_INSUFFICIENT_BUFFER;
  }

  LSTATUS lstatus = register_interface_guid(EMAGIC_VID, EMI26_PID, guid);
  return lstatus == ERROR_SUCCESS ? register_interface_guid(EMAGIC_VID, EMI26B_PID, guid) : lstatus;
}
