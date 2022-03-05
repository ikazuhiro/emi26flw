# emagic emi 2|6 firmware loader for WinUSB

emagic emi 2|6 is a USB audio interface.  There is no driver for 64bit Windows.  Linux has a firmware loader for the device.  After loading firmware, it changes into USB Audio class compliant device.  I've ported Linux's firmware loader as WinUSB client.

## Build
I confirmed Mingw-w64 on MSYS2.

```sh
gcc -municode -O3 device.c emi26.c main.c ihex.c emi26r.c \
  -lwinusb -lcfgmgr32 -l pathcch -l shlwapi -l ole32 -o emi26flw.exe
```

## Usage
### Prepare firmwares
You need firmware files to be loaded.  There are in [Linux firmware's repository](https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/tree/emi26).  `bitstream.fw`, `firmware.fw` and `loader.fw` are needed.  you should place them in `emi26` directory at the same place with `emi26flw.exe` binary.

### Install device as WinUSB device
See [Installing WinUSB by specifying the system-provided device class](https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-installation) section of Microsoft's document.

### Generate a device interface GUID
Run `emi26flw.exe` with `-r` option under **administrator privileges**.

### Reconnect the device
Just do it.

### Run firmware loader
Simply run `emi26flw.exe`.  If you reconnect the device at the same physical port, only repeating this step is required.

## License
[GPL2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)
