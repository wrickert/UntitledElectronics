# Confirmed Solutions

## SK6812

The working SK6812 fix for this board was:

- use the known-good SK6812 driver pattern from `code/ch585_nfca_usb`
- set `SK6812_PROFILE` to `5`
- keep `SK6812_IS_RGBW` enabled
- keep `SK6812_W_BRIGHT` at `0`
- drive the data line low at boot with `sk6812_set_grbw(0, 0, 0, 0)`

This corrected the "always white" failure mode.

## NFC Pin Note

This board does not have a `PB8` status LED.
On CH585, `PB8/PB9` are part of the NFC analog path, and driving `PB8` as a
regular LED pin can interfere with weak tag detection.

## Audio Status

Audio on `PB0` is still not confirmed in the integrated firmware. The current
debug firmware keeps the boot beep path and uses `20mA` push-pull drive on
`PB0`, but this is not yet verified on hardware.
