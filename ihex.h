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


#include <windows.h>

#pragma pack(2)
typedef struct {
  ULONG addr;
  USHORT len;
  UCHAR data[0];
} ihex_binrec;
#pragma pack()

static inline USHORT ihex_binrec_length(const ihex_binrec* binrec) {
  return _byteswap_ushort(binrec->len);
}

static inline ULONG ihex_binrec_address(const ihex_binrec* binrec) {
  return _byteswap_ulong(binrec->addr);
}

static inline const ihex_binrec* ihex_next_binrec(const ihex_binrec* rec) {
  rec = ((void *)rec) + ((ihex_binrec_length(rec) + sizeof(ihex_binrec) + 3) & ~3);
  return rec->len ? rec : NULL;
}


BOOL ihex_validate(const UCHAR *data, ULONG length);
PUCHAR ihex_load(const PCWSTR filename, PULONG size);
