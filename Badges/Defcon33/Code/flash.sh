#!/bin/bash

PORT="/dev/ttyACM0"
FIRMWARE="ESP32_GENERIC_S3-20250415-v1.25.0 (1).bin"
AMPY="ampy -p $PORT"
WAV="../Audio/Wav/listen.wav"

echo "=== 1. Flashing firmware with esptool ==="
esptool.py --chip esp32-s3 -p "$PORT" write_flash 0 "$FIRMWARE"

echo "=== Waiting for ESP32-S3 to reboot (5s) ==="
sleep 5

echo "=== 2. Uploading main.py ==="
$AMPY put main.py

echo "=== 3. Uploading lis2hh12.py ==="
$AMPY put lis2hh12.py

echo "=== 4. Uploading AW20072.py ==="
$AMPY put AW20072.py

echo "=== 5. Uploading listen.wav ==="
$AMPY put "$WAV"

echo "=== All done! ==="

echo "=== Rebooting MicroPython ==="
mpremote connect $PORT reset
