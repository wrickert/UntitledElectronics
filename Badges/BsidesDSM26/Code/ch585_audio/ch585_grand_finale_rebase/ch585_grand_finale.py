#!/usr/bin/env python3
"""
Host-side helper for the CH585 NFCA USB firmware.

Notes
- Requires pyusb (pip install pyusb) and a libusb backend.
- On Linux, you will likely need a udev rule so non-root users can open the device:
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
USB_PKT_DEBUG = 0xA4
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
CH_USB_PACKET_SIZE  = 64        # packet size (USB full-speed bulk max packet is 64 bytes)
CH_USB_TIMEOUT_MS   = 100       # timeout for normal USB operations

CH_CMD_REBOOT       = 0xa2
CH_STR_REBOOT       = (CH_CMD_REBOOT, 0x01, 0x00, 0x01)

def _parse_int(s: str) -> int:
    # Accept hex ("0x1209") or decimal ("4617").
    return int(s, 0)

def _list_devices():
    devs = list(usb.core.find(find_all=True) or [])
    if not devs:
        print("No USB devices visible to libusb/pyusb.")
        return 1

    for d in devs:
        vidpid = f"{d.idVendor:04x}:{d.idProduct:04x}"
        # Strings may require permissions; keep best-effort.
        mfg = prod = ser = None
        try:
            mfg = usb.util.get_string(d, d.iManufacturer) if d.iManufacturer else None
            prod = usb.util.get_string(d, d.iProduct) if d.iProduct else None
            ser = usb.util.get_string(d, d.iSerialNumber) if d.iSerialNumber else None
        except Exception:
            pass
        extra = " ".join(x for x in [mfg, prod, ser] if x)
        if extra:
            print(f"{vidpid}  {extra}")
        else:
            print(vidpid)
    return 0

def _open_device(vid: int, pid: int):
    dev = usb.core.find(idVendor=vid, idProduct=pid)
    if dev is None:
        return None
    # Some hosts require an explicit configuration.
    try:
        dev.set_configuration()
    except Exception:
        # If permissions are missing, later read/write will fail with a clearer error.
        pass
    return dev

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bootloader', help='Reboot to bootloader', action='store_true')
    parser.add_argument('--list', help='List USB devices visible to pyusb/libusb', action='store_true')
    parser.add_argument('--vid', help='USB vendor id (default: 0x1209)', type=_parse_int, default=CH_USB_VENDOR_ID)
    parser.add_argument('--pid', help='USB product id (default: 0xd035)', type=_parse_int, default=CH_USB_PRODUCT_ID)
    parser.add_argument('--ep-in', help='IN endpoint (default: 0x81)', type=_parse_int, default=CH_USB_EP_IN)
    parser.add_argument('--ep-out', help='OUT endpoint (default: 0x02)', type=_parse_int, default=CH_USB_EP_OUT)
    parser.add_argument('--timeout-ms', help='USB timeout in ms (default: 100)', type=int, default=CH_USB_TIMEOUT_MS)
    args = parser.parse_args()

    if args.list:
        raise SystemExit(_list_devices())

    device = _open_device(args.vid, args.pid)
    if device is None:
        print(f"ch585 NFCA not found (looking for {args.vid:04x}:{args.pid:04x})")
        print("Tip: run with --list to see what pyusb can see.")
        raise SystemExit(0)

    # Clear any pending data in the pipes
    try:
        device.read(args.ep_in, CH_USB_PACKET_SIZE, 10)
    except:
        pass

    if args.bootloader:
        print('rebooting to bootloader')
        try:
            device.write(args.ep_out, CH_STR_REBOOT)
            sleep(.3)
        except Exception as e:
            print(f"Command sent (device may have reset already): {e}")
    else:
        print("Polling for card (Ctrl+C to exit)...")
        poll_loop(device, args.ep_in, args.ep_out, args.timeout_ms)
    
    print('done')

def poll_loop(device, ep_in, ep_out, timeout_ms):
    while True:
        try:
            # Short timeout read to check for data
            r = device.read(ep_in, CH_USB_PACKET_SIZE, timeout_ms).tobytes()

            # Check for Heartbeat
            if r.startswith(b'nfca585'):
                pass
            elif r and r[0] == USB_PKT_DEBUG:
                # Debug telemetry (see firmware USB_PKT_DEBUG).
                # Layout:
                #  0  type
                #  1  comm_status
                #  2  intf_status lo
                #  3  intf_status hi
                #  4..7 recv_bits (le)
                #  8..9 ant_adc (le)
                # 10..11 lpcd_base (le)
                # 12 lpcd_hit
                # 13 nfc_cmd
                # 14 nfc_status
                comm_status = r[1]
                intf_status = r[2] | (r[3] << 8)
                recv_bits = r[4] | (r[5] << 8) | (r[6] << 16) | (r[7] << 24)
                ant_adc = r[8] | (r[9] << 8)
                lpcd_base = r[10] | (r[11] << 8)
                lpcd_hit = r[12]
                nfc_cmd = r[13]
                nfc_status = r[14]
                print(f"[DBG] ant_adc={ant_adc:4d} lpcd_base={lpcd_base:4d} lpcd_hit={lpcd_hit} "
                      f"comm={comm_status} intf=0x{intf_status:04x} recv_bits={recv_bits} "
                      f"cmd=0x{nfc_cmd:02x} status=0x{nfc_status:02x}")
            else:
                # Check for ATS (Answer To Select) / RATS Response
                # ATS usually starts with length byte, but we can detect based on context
                print(f"\n[+] Card Detected! Data: {r.hex().upper()}")
                handle_card_interaction(device, ep_in, ep_out, r)
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


def dump_ntag215(device, ep_in, initial_packet):
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
            packet = device.read(ep_in, CH_USB_PACKET_SIZE, 3000).tobytes()
        except usb.core.USBTimeoutError:
            print("	[!] Timed out waiting for remaining NTAG dump packets.")
            break

    if pages:
        print("	NTAG215 pages:")
        for page_no in range(NTAG215_TOTAL_PAGES):
            if page_no in pages:
                print(f"	  {page_no:03d}: {pages[page_no].hex(' ')}")


def handle_card_interaction(device, ep_in, ep_out, initial_data):
    if initial_data and initial_data[0] == USB_PKT_NTAG_VERSION:
        dump_ntag215(device, ep_in, initial_data)
        return

    print(f"	RATS/ATS: {initial_data.hex(':')}")

    TOPAY = bytes([0x00, 0xA4, 0x04, 0x00, 0x0E]) + b'2PAY.SYS.DDF01' + bytes([0x00])

    try:
        print(f"	Sending 2PAY Select...")
        device.write(ep_out, TOPAY)

        # Wait longer for card response
        r = device.read(ep_in, CH_USB_PACKET_SIZE, 3000).tobytes()

        if r.startswith(b'nfca585'):
            print("	[!] Heartbeat received during transaction - Card likely lost.")
        else:
            print(f"	2PAY Response: {r.hex(':')}")

    except usb.core.USBTimeoutError:
        print("	[!] Card interaction timed out.")

if __name__ == '__main__':
    main()
