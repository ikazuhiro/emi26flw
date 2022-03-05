/*
  Copyright (c) 2022  Kazuhiro Ito (kzhr@d1.dion.ne.jp)

  Original file is include/linux/ihex.h of Linux kernel.

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

#include "ihex.h"

/* main.c */
void errnum_printf_message(PCWSTR string, DWORD errnum);


/* Return TRUE data is correct ihex binary. */
BOOL ihex_validate(const UCHAR *data, ULONG length) {
  const ihex_binrec *binrec = (ihex_binrec *)data;
  ULONG size = sizeof(ihex_binrec);
  ULONG offset = 0;

  while (binrec && binrec < (ihex_binrec *)&data[length - sizeof(ihex_binrec)]) {
    offset = ihex_binrec_address(binrec);

    size += ((ihex_binrec_length(binrec) + sizeof(ihex_binrec) + 3) & ~3);
    binrec = ihex_next_binrec(binrec);
  }

  if (binrec || size != length) return FALSE;

  return TRUE;
}

static PUCHAR file_load(PCWSTR filename, PULONG size) {
  PUCHAR pBuffer = NULL;

  HANDLE hInfile =
    CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL || FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hInfile == INVALID_HANDLE_VALUE) {
    errnum_printf_message(L"Error in opening file", GetLastError());
    goto error;
  }

  LARGE_INTEGER fileSize;
  ULONG ulFileSize;
  if (GetFileSizeEx(hInfile, &fileSize) == 0) {
    errnum_printf_message(L"Error in getting file size", GetLastError());
    goto error;
  } else if (fileSize.QuadPart >= 0x7fffffff) {
    wprintf (L"Too large file size (%lld bytes).\n", (LONGLONG) fileSize.QuadPart);
    goto error;
  }
  ulFileSize = (ULONG)fileSize.QuadPart;

  pBuffer = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)ulFileSize);
  if (!pBuffer) {
    errnum_printf_message(L"Error in allocating memory", GetLastError());
    goto error;
  }

  DWORD bytes;
  if (!ReadFile(hInfile, pBuffer, ulFileSize, &bytes, NULL)) {
    wprintf (L"Read failure.\n");
    goto error;
  }

  if (size) *size = (ULONG)bytes;
  CloseHandle(hInfile);
  return pBuffer;

 error:
  if (size) *size = 0;
  if (pBuffer) HeapFree(GetProcessHeap(), 0, pBuffer);
  if (hInfile != INVALID_HANDLE_VALUE) CloseHandle(hInfile);
  return NULL;
}


PUCHAR ihex_load(const PCWSTR filename, PULONG size) {
  ULONG tmpSize;
  wprintf (L"Loading firmware from %S\n", filename);

  PUCHAR data = file_load(filename, &tmpSize);

  if (!data) {
    wprintf (L"Failed to load %S\n", filename);
    goto error;
  }

  if (!ihex_validate(data, tmpSize)) {
    wprintf (L"Validation failed.\n");
    goto error;
  }

  if (size) *size = tmpSize;
  return data;

 error:
  if (data) HeapFree(GetProcessHeap(), 0, data);
  if (size) *size = 0;
  return NULL;
}
