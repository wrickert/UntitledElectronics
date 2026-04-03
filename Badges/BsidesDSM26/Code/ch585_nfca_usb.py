#!/usr/bin/env python3
"""
requires pyusb, which should be pippable
SUBSYSTEM=="usb", ATTR{idVendor}=="1209", ATTR{idProduct}=="d035", MODE="666"
sudo udevadm control --reload-rules && sudo udevadm trigger
"""
import argparse
import usb.core
import usb.util
from time import sleep

USB_PKT_NTAG_VERSION = 0xA1
USB_PKT_NTAG_PAGE_DATA = 0xA2
USB_PKT_NTAG_DONE = 0xA3
NTAG215_TOTAL_PAGES = 135

NTAG_DONE_STATUS = {
    0: "ok",
    1: "GET_VERSION failed",
    2: "tag is not NTAG215",
    3: "READ failed during dump",
}

CH_USB_VENDOR_ID    = 0x1209    # VID
CH_USB_PRODUCT_ID   = 0xd035    # PID
CH_USB_INTERFACE    = 0         # interface number
CH_USB_EP_IN        = 0x81      # endpoint for reply transfer in
CH_USB_EP_OUT       = 0x02      # endpoint for command transfer out
CH_USB_PACKET_SIZE  = 256       # packet size
CH_USB_TIMEOUT_MS   = 100       # timeout for normal USB operations

CH_CMD_REBOOT       = 0xa2
CH_STR_REBOOT       = (CH_CMD_REBOOT, 0x01, 0x00, 0x01)

# Find the device
device = usb.core.find(idVendor=CH_USB_VENDOR_ID, idProduct=CH_USB_PRODUCT_ID)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bootloader', help='Reboot to bootloader', action='store_true')
    args = parser.parse_args()

    if device is None:
        print("ch585 NFCA not found")
        exit(0)

    # Clear any pending data in the pipes
    try:
        device.read(CH_USB_EP_IN, CH_USB_PACKET_SIZE, 10)
    except:
        pass

    if args.bootloader:
        print('rebooting to bootloader')
        try:
            device.write(CH_USB_EP_OUT, CH_STR_REBOOT)
            sleep(.3)
        except Exception as e:
            print(f"Command sent (device may have reset already): {e}")
    else:
        print("Polling for card (Ctrl+C to exit)...")
        poll_loop()
    
    print('done')

def poll_loop():
    while True:
        try:
            # Short timeout read to check for data
            r = device.read(CH_USB_EP_IN, CH_USB_PACKET_SIZE, CH_USB_TIMEOUT_MS).tobytes()

            # Check for Heartbeat
            if r.startswith(b'nfca585'):
                pass
            else:
                # Check for ATS (Answer To Select) / RATS Response
                # ATS usually starts with length byte, but we can detect based on context
                print(f"\n[+] Card Detected! Data: {r.hex().upper()}")
                handle_card_interaction(r)
                print("[-] Interaction finished, returning to poll.")

        except usb.core.USBTimeoutError:
            continue
        except usb.core.USBError as e:
            if e.errno == 19: # No such device (unplugged)
                print("\nDevice disconnected.")
                break
            continue
        except KeyboardInterrupt:
            break

def decode_ntag_product(version):
    size_code = version[6]
    if size_code == 0x0F:
        return "NTAG213"
    if size_code == 0x11:
        return "NTAG215"
    if size_code == 0x13:
        return "NTAG216"
    return f"NTAG21x(size=0x{size_code:02X})"


def dump_ntag215(initial_packet):
    pages = {}
    packet = initial_packet

    while True:
        pkt_type = packet[0]

        if pkt_type == USB_PKT_NTAG_VERSION:
            uid_len = packet[1]
            uid = packet[2:2 + uid_len]
            version = packet[12:20]
            print(f"	Detected {decode_ntag_product(version)} UID={uid.hex(':')} version={version.hex(':')}")
        elif pkt_type == USB_PKT_NTAG_PAGE_DATA:
            start_page = packet[1]
            page_count = packet[2]
            payload = packet[3:3 + (page_count * 4)]
            for idx in range(page_count):
                page_no = start_page + idx
                pages[page_no] = payload[idx * 4:(idx + 1) * 4]
        elif pkt_type == USB_PKT_NTAG_DONE:
            status = packet[1]
            print(f"	NTAG dump status: {NTAG_DONE_STATUS.get(status, f'unknown ({status})')}")
            break

        try:
            packet = device.read(CH_USB_EP_IN, CH_USB_PACKET_SIZE, 3000).tobytes()
        except usb.core.USBTimeoutError:
            print("	[!] Timed out waiting for remaining NTAG dump packets.")
            break

    if pages:
        print("	NTAG215 pages:")
        for page_no in range(NTAG215_TOTAL_PAGES):
            if page_no in pages:
                print(f"	  {page_no:03d}: {pages[page_no].hex(' ')}")


def handle_card_interaction(initial_data):
    if initial_data and initial_data[0] == USB_PKT_NTAG_VERSION:
        dump_ntag215(initial_data)
        return

    print(f"	RATS/ATS: {initial_data.hex(':')}")

    TOPAY = bytes([0x00, 0xA4, 0x04, 0x00, 0x0E]) + b'2PAY.SYS.DDF01' + bytes([0x00])

    try:
        print(f"	Sending 2PAY Select...")
        device.write(CH_USB_EP_OUT, TOPAY)

        # Wait longer for card response
        r = device.read(CH_USB_EP_IN, CH_USB_PACKET_SIZE, 3000).tobytes()

        if r.startswith(b'nfca585'):
            print("	[!] Heartbeat received during transaction - Card likely lost.")
        else:
            print(f"	2PAY Response: {r.hex(':')}")

    except usb.core.USBTimeoutError:
        print("	[!] Card interaction timed out.")

if __name__ == '__main__':
    main()
