# ch585_audio

Standalone `PB0` audio test for CH585.

This firmware is intentionally minimal:
- `PB0` is the only audio output pin
- it drives `PB0` directly with square-wave notes
- there is no NFC, USB, SK6812, or secondary status LED logic

## Purpose

This project is the clean audio reference for the board.

If `ch585_audio` makes sound and `ch585_grand_finale` does not, the problem is
integration. If `ch585_audio` also stays silent, the problem is in the `PB0`
audio path, wiring, amp behavior, or board assumptions rather than the larger
firmware.

## Wiring

- `PB0` -> amp input
- `GND` -> amp ground

## Behavior

After boot, it continuously repeats a short four-note test pattern on `PB0`.
This is still a direct GPIO-square-wave test, just at audible note frequencies.

## Build

```sh
make ch585_audio.bin
```
