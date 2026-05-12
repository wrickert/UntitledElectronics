# ch585_grand_finale

Combined CH585 firmware base for:
- NFCA / NTAG reader over USB
- SK6812 status output
- simple PWM audio cue on `PB0`

Notes:
- this board does not use a `PB8` status LED
- `PB8/PB9` are part of the CH585 NFC analog path and should be left alone for NFC work
- the normal firmware now plays a `PB0` debug beep at boot

This project is intended to be the integration point for the final firmware,
including future "notes on tag" playback.
