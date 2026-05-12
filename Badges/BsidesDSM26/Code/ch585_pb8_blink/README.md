# ch585_pb8_blink

Minimal CH585 firmware that toggles PB8 (about 2Hz).

Build + flash:

```sh
cd ch585_pb8_blink
make
```

Notes
- Enter ROM bootloader mode (PB22 / BOOT button) before flashing.
- If your eval board is not CH585M (QFN48), override:
  `make TARGET_MCU_PACKAGE=CH585F` (etc).

