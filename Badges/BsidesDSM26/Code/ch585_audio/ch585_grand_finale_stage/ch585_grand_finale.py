#!/usr/bin/env python
"""
requires pyusb, which should be pippable
SUBSYSTEM=="usb", ATTR{idVendor}=="1209", ATTR{idProduct}=="d035", MODE="666"
sudo udevadm control --reload-rules && sudo udevadm trigger
"""
import argparse
import usb.core
import usb.util
from time import sleep

CH_USB_VENDOR_ID    = 0x1209    # VID
CH_USB_PRODUCT_ID   = 0xd035    # PID
CH_USB_INTERFACE    = 0         # interface number
CH_USB_EP_IN        = 0x81      # endpoint for reply transfer in
CH_USB_EP_OUT       = 0x02      # endpoint for command transfer out
CH_USB_PACKET_SIZE  = 512       # max framed message size (header + payload)
CH_USB_TIMEOUT_MS   = 1200      # timeout for normal USB operations

CH_CMD_REBOOT       = 0xa2
CH_STR_REBOOT       = bytes([CH_CMD_REBOOT, 0x01, 0x00, 0x01])

# Find the device
device = usb.core.find(idVendor=CH_USB_VENDOR_ID, idProduct=CH_USB_PRODUCT_ID)

def ensure_claimed():
    # Avoid detach_kernel_driver (often requires elevated privileges and shouldn't be needed for a vendor-class interface).
    # Just ensure a configuration is set and claim the interface.
    try:
        device.set_configuration()
    except usb.core.USBError as e:
        # If configuration is already set, or we lack permissions, proceed; reads may still work.
        if getattr(e, "errno", None) == 13:
            print("USB permission denied; install the included `99-nfca585.rules` or run with sudo.")
    try:
        cfg = device.get_active_configuration()
        intf = cfg[(CH_USB_INTERFACE, 0)]
        usb.util.claim_interface(device, intf.bInterfaceNumber)
    except usb.core.USBError as e:
        if getattr(e, "errno", None) == 13:
            print("USB permission denied; install the included `99-nfca585.rules` or run with sudo.")

def write_msg(payload: bytes):
    if payload is None:
        payload = b""
    if len(payload) > 510:
        raise ValueError("payload too large (max 510 bytes)")
    header = len(payload).to_bytes(2, "little")
    device.write(CH_USB_EP_OUT, header + payload)

def read_exact(n: int, timeout_ms: int) -> bytes:
    out = bytearray()
    while len(out) < n:
        chunk = device.read(CH_USB_EP_IN, n - len(out), timeout_ms).tobytes()
        if not chunk:
            break
        out += chunk
    return bytes(out)

def read_msg(timeout_ms: int) -> bytes:
    # Preferred framing (USBFS): firmware sends a short 2-byte header packet first,
    # then host reads exactly payload_len bytes.
    #
    # Back-compat: if we get anything other than 2 bytes, treat it as a legacy
    # single-packet payload (old USBHS behavior).
    first = device.read(CH_USB_EP_IN, 64, timeout_ms).tobytes()
    if len(first) != 2:
        return first
    payload_len = int.from_bytes(first, "little")
    if payload_len == 0:
        return b""
    return read_exact(payload_len, timeout_ms)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bootloader', help='Reboot to bootloader', action='store_true')
    args = parser.parse_args()

    if device is None:
        print("ch585 NFCA not found")
        exit(0)

    ensure_claimed()

    # Clear any pending data in the pipes
    try:
        device.read(CH_USB_EP_IN, 64, 10)
    except:
        pass

    if args.bootloader:
        print('rebooting to bootloader')
        try:
            write_msg(CH_STR_REBOOT)
            sleep(.3)
        except Exception as e:
            print(f"Command sent (device may have reset already): {e}")
    else:
        print("Polling for card (Ctrl+C to exit)...")
        poll_loop()
    
    print('done')

def poll_loop():
    last_hb_print = 0.0
    last_any_print = 0.0
    last_err_print = 0.0
    while True:
        try:
            r = read_msg(CH_USB_TIMEOUT_MS)

            # Heartbeat is a plain string payload.
            if r.startswith(b'nfca585'):
                now = __import__("time").time()
                if now - last_hb_print > 1.0:
                    print(".", end="", flush=True)
                    last_hb_print = now
                continue

            # Card info event from firmware: 0xA5, uid_len, uid..., atqa(2 LE), sak(1)
            if len(r) >= 1 and r[0] == 0xA5 and len(r) >= 1 + 1 + 2 + 1:
                uid_len = r[1]
                if len(r) >= 2 + uid_len + 3:
                    uid = r[2:2+uid_len]
                    atqa = int.from_bytes(r[2+uid_len:2+uid_len+2], "little")
                    sak = r[2+uid_len+2]
                    print(f"\n[+] Card Detected! UID={uid.hex().upper()} ATQA=0x{atqa:04X} SAK=0x{sak:02X}")
                    # Only attempt ISO-DEP/APDU flow if firmware sends ATS next; otherwise keep polling.
                    continue

            print(f"\n[+] Card Detected! Data: {r.hex().upper()}")
            handle_card_interaction(r)
            print("[-] Interaction finished, returning to poll.")

        except usb.core.USBTimeoutError:
            now = __import__("time").time()
            if now - last_any_print > 5.0:
                print("\n(no USB data)", flush=True)
                last_any_print = now
            continue
        except usb.core.USBError as e:
            if e.errno == 13:
                print("\nUSB permission denied; install `99-nfca585.rules` or run with sudo.")
                break
            if e.errno == 19: # No such device (unplugged)
                print("\nDevice disconnected.")
                break
            now = __import__("time").time()
            if now - last_err_print > 2.0:
                print(f"\nUSB error: {e}", flush=True)
                last_err_print = now
            continue
        except KeyboardInterrupt:
            break

def handle_card_interaction(initial_data):
    print(f"\tRATS/ATS: {initial_data.hex(':')}")

    TOPAY = bytes([0x00, 0xA4, 0x04, 0x00, 0x0E]) + b'2PAY.SYS.DDF01' + bytes([0x00])

    try:
        print(f"\tSending 2PAY Select...")
        write_msg(TOPAY)

        # Wait longer for card response
        r = read_msg(3000)

        if r.startswith(b'nfca585'):
            print("\t[!] Heartbeat received during transaction - Card likely lost.")
        else:
            print(f"\t2PAY Response: {r.hex(':')}")

    except usb.core.USBTimeoutError:
        print("\t[!] Card interaction timed out.")

if __name__ == '__main__':
    main()
