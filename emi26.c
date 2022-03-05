/*
  emagic emi 2|6 firmware loader for WinUSB

  Copyright (c) 2022  Kazuhiro Ito (kzhr@d1.dion.ne.jp)

  Original file is drivers/usb/misc/emi26.c of Linux kernel
  Copyright (C) 2002  Tapio Laxström (tapio.laxstrom@iptime.fi)

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
#include <ntdef.h>
#include <winusb.h>
#include <ntstatus.h>
#include <pathcch.h>

#include "ihex.h"

/* main.c */
void errnum_printf_message(PCWSTR string, DWORD errnum);


/* Vendor specific request code for Anchor Upload/Download (This one is implemented in the core) */
#define ANCHOR_LOAD_INTERNAL	0xA0

/* This command is not implemented in the core. Requires firmware */
#define ANCHOR_LOAD_EXTERNAL	0xA3

/* This command is not implemented in the core. Requires firmware. Emagic extension */
#define ANCHOR_LOAD_FPGA	0xA5

/* This is the highest internal RAM address for the AN2131Q */
#define MAX_INTERNAL_ADDRESS	0x1B3F

/* EZ-USB Control and Status Register.  Bit 0 controls 8051 reset */
#define CPUCS_REG		0x7F92

/* AFAIT, available maximum value is 1030 for emi 2|6 */
#define FW_LOAD_SIZE (1024)


/* emi2|6 firmware neames */
static const wchar_t emi26_name_bitstream_fw[] = L"bitstream.fw";
static const wchar_t emi26_name_firmware_fw[]  = L"firmware.fw";
static const wchar_t emi26_name_loader_fw[]    = L"loader.fw";
static PUCHAR emi26_bitstream_fw = NULL;
static PUCHAR emi26_firmware_fw = NULL;
static PUCHAR emi26_loader_fw = NULL;

#define INTERNAL_RAM(address)   (address <= MAX_INTERNAL_ADDRESS)


static PUCHAR emi26_load_file(PCWSTR directory, PCWSTR file, PULONG size) {
  WCHAR filename[MAX_PATH];

  HRESULT hResult = PathCchCombine(filename, MAX_PATH, directory, file);
  if (hResult != S_OK) {
    wprintf(L"Too long directory name\n");
    return NULL;
  }

  return ihex_load(filename, size);
}

NTSTATUS emi26_load_files(PCWSTR directory) {
  ULONG size;

  if (!(emi26_bitstream_fw = emi26_load_file(directory, emi26_name_bitstream_fw, &size))) goto error;
  if (!(emi26_firmware_fw  = emi26_load_file(directory, emi26_name_firmware_fw, &size))) goto error;
  if (!(emi26_loader_fw    = emi26_load_file(directory, emi26_name_loader_fw, &size))) goto error;

  return STATUS_SUCCESS;

 error:
  if (emi26_bitstream_fw) HeapFree(GetProcessHeap(), 0, emi26_bitstream_fw);
  if (emi26_firmware_fw)  HeapFree(GetProcessHeap(), 0, emi26_firmware_fw);
  if (emi26_loader_fw)    HeapFree(GetProcessHeap(), 0, emi26_loader_fw);
  emi26_bitstream_fw = emi26_firmware_fw = emi26_loader_fw = NULL;
  return STATUS_FILE_INVALID;
}

NTSTATUS emi26_writememory(WINUSB_INTERFACE_HANDLE interfaceHandle, USHORT address,
			   PCUCHAR data, ULONG length, BYTE request, PULONG psentBytes) {
  WINUSB_SETUP_PACKET setupPacket;
  setupPacket.RequestType = 0x40;
  setupPacket.Request = request;
  setupPacket.Value = address;
  setupPacket.Index = 0;

  if (length > 4096) {
    wprintf(L"Exceed the buffer size limit (4096) for WinUsb_ControlTransfer, %dBytes specified.\n", length);
  }

  if (WinUsb_ControlTransfer(interfaceHandle, setupPacket, (PUCHAR)data, length, psentBytes, NULL))
    return STATUS_SUCCESS;

  return GetLastError();
}

static NTSTATUS emi26_set_reset(WINUSB_INTERFACE_HANDLE interfaceHandle, UCHAR reset_bit) {
  ULONG sentBytes;
  NTSTATUS ntstatus = emi26_writememory(interfaceHandle, CPUCS_REG, &reset_bit, 1, 0xa0, &sentBytes);

  if (!NT_SUCCESS(ntstatus)) errnum_printf_message(L"Failed to reset device", ntstatus);
  return ntstatus;
}


static NTSTATUS emi26_load_fw(WINUSB_INTERFACE_HANDLE interfaceHandle, const PUCHAR firmware, const BYTE request) {
  ULONG sentBytes;
  NTSTATUS ntstatus;
  const ihex_binrec *binrec = (const ihex_binrec *)firmware;
  UCHAR buffer[FW_LOAD_SIZE];

  while (binrec) {

    int size = 0;
    ULONG address = 0;
    while (binrec && ihex_binrec_length(binrec) + size <= FW_LOAD_SIZE &&
	   (size == 0 || address + size == ihex_binrec_address(binrec))) {
      memcpy(buffer + size, binrec->data, ihex_binrec_length(binrec));
      if (!address) address = ihex_binrec_address(binrec);
      size += ihex_binrec_length(binrec);
      binrec = ihex_next_binrec(binrec);
    }

    if (size) {
      ntstatus = emi26_writememory(interfaceHandle, address, buffer, size, request, &sentBytes);
    } else {
      size = ihex_binrec_length(binrec);
      ntstatus = emi26_writememory(interfaceHandle, (USHORT)ihex_binrec_address(binrec),
				   binrec->data, size, request, &sentBytes);
      binrec = ihex_next_binrec(binrec);
    }

    if (!NT_SUCCESS(ntstatus)) {
      errnum_printf_message(L"Failed to upload firmware", ntstatus);
      return ntstatus;
    } else if (sentBytes != size) {
      wprintf(L"Not all data ware transferred.\n");
      return STATUS_ADAPTER_HARDWARE_ERROR;
    }
  }

  return ntstatus;
}

static NTSTATUS emi26_load_loader_fw(WINUSB_INTERFACE_HANDLE interfaceHandle) {
  NTSTATUS ntstatus = emi26_load_fw(interfaceHandle, emi26_loader_fw, ANCHOR_LOAD_INTERNAL);
  if (!NT_SUCCESS(ntstatus)) {
    return ntstatus;
  }

  ntstatus = emi26_set_reset(interfaceHandle, 0);
  if (!NT_SUCCESS(ntstatus)) {
    return ntstatus;
  }

  Sleep(250);
  return ntstatus;
}

NTSTATUS emi26_load_firmware(WINUSB_INTERFACE_HANDLE interfaceHandle, PCWSTR directory)
{
  ULONG sentBytes;
  const ihex_binrec *binrec;
  NTSTATUS  ntstatus;

  if (!emi26_firmware_fw) {
    ntstatus = emi26_load_files(directory);
    if (!NT_SUCCESS(ntstatus)) {
      wprintf(L"Failed to load firmware files.\n");
      return ntstatus;
    }
  }

  wprintf(L"Uploading firmwares.\n");
  ntstatus = emi26_set_reset(interfaceHandle, 1);
  if (!NT_SUCCESS(ntstatus)) return ntstatus;

  wprintf(L"Uploading %S.\n", emi26_name_loader_fw);
  ntstatus = emi26_load_loader_fw(interfaceHandle);
  if (!NT_SUCCESS(ntstatus)) return ntstatus;

  wprintf(L"Uploading %S.\n", emi26_name_bitstream_fw);
  ntstatus = emi26_load_fw(interfaceHandle, emi26_bitstream_fw, ANCHOR_LOAD_FPGA);
  if (!NT_SUCCESS(ntstatus)) return ntstatus;

  ntstatus = emi26_set_reset(interfaceHandle, 1);
  if (!NT_SUCCESS(ntstatus)) return ntstatus;

  wprintf(L"Re-uploading %S.\n", emi26_name_loader_fw);
  ntstatus = emi26_load_loader_fw(interfaceHandle);
  if (!NT_SUCCESS(ntstatus)) return ntstatus;

  wprintf(L"Uploading %S into external storage.\n", emi26_name_firmware_fw);
  binrec = (const ihex_binrec*)emi26_firmware_fw;
  while (binrec && INTERNAL_RAM(ihex_binrec_address(binrec))) {
    binrec = ihex_next_binrec(binrec);
  }
  if (binrec) {
    ntstatus = emi26_load_fw(interfaceHandle, (PUCHAR)binrec, ANCHOR_LOAD_EXTERNAL);
    if (!NT_SUCCESS(ntstatus)) return ntstatus;
  } else {
    /* ntstatus.h of MingW doesn't have th definition of STATUS_FIRMWARE_IMAGE_INVALID yet. */
    /* return STATUS_FIRMWARE_IMAGE_INVALID; */
    return ((NTSTATUS)0xC0000485);
  }

  ntstatus = emi26_set_reset(interfaceHandle, 1);
  if (!NT_SUCCESS(ntstatus)) return ntstatus;

  ((ihex_binrec *)binrec)->addr = 0;
  ((ihex_binrec *)binrec)->len = 0;

  wprintf(L"Uploading %S into internal storage.\n", emi26_name_firmware_fw);
  ntstatus = emi26_load_fw(interfaceHandle, emi26_firmware_fw, ANCHOR_LOAD_INTERNAL);
  if (!NT_SUCCESS(ntstatus))  return ntstatus;

  ntstatus = emi26_set_reset(interfaceHandle, 0);
  if (!NT_SUCCESS(ntstatus)) return ntstatus;

  wprintf(L"Done.\n");
  return ntstatus;
}
