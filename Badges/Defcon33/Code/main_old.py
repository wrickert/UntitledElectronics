# main.py

from machine import I2C, Pin, I2S
from lis2hh12 import LIS2HH12
import time
import array
import ustruct
import math
import random

# I2S Config (update id and pins if needed for your board)
i2s = I2S(
    0,                          # I2S peripheral id (try 0 or 1)
    sck=Pin(48),                # Bit clock (BCLK)
    ws=Pin(45),                 # Word select (LRCLK)
    sd=Pin(47),                 # Serial data (DIN)
    mode=I2S.TX,
    bits=16,
    format=I2S.MONO,            # or I2S.STEREO if needed
    rate=8000, #22050,smaller file size       Audio sample rate
    ibuf=20000                  # Internal buffer length
)

# Setup I2C
i2c = I2C(0, scl=Pin(10), sda=Pin(9), freq=100000)
AW_ADDR = 0x3A

# Setup Pins
gpio17 = Pin(17, Pin.OUT)
lights = Pin(12, Pin.IN, Pin.PULL_UP)
audioEN = Pin(21, Pin.OUT)

# Sensitivity threshold (tweak this!)
SWING_THRESHOLD = 19  # In g's. Try 2-4, adjust as needed

# --- Basic read/write helpers for the aw20072 ---
def aw_write(reg, data):
    i2c.writeto_mem(AW_ADDR, reg, bytes([data]))

def aw_read(reg):
    return i2c.readfrom_mem(AW_ADDR, reg, 1)[0]

acc = LIS2HH12(i2c)


def aw20072_init():
    # Page 0
    aw_write(0xF0, 0xC0)
    aw_write(0x02, 0x01)  # software reset
    time.sleep(0.002)
    aw_write(0x03, 0x18)  # Imax, ALLON = 0
    aw_write(0x80, 0x02)  # SIZE config
    aw_write(0x01, 0x00)  # enter active mode

def aw20x_MaxOn():
    # Page 1 - set all color values to 0x3F
    aw_write(0xF0, 0xC1)
    for i in range(0x24):
        aw_write(i, 0x3F)

    # Page 2 - brightness registers (0xFF full)
    aw_write(0xF0, 0xC2)
    for i in range(0x24):
        aw_write(i, 0xFF)

def aw20x_Off():
    # Page 1 - set all color values to 0x3F
    aw_write(0xF0, 0xC1)
    for i in range(0x24):
        aw_write(i, 0x3F)

    # Page 2 - brightness registers (0xFF full)
    aw_write(0xF0, 0xC2)
    for i in range(0x24):
        aw_write(i, 0x00)

def playAudio(num):
    print("Playing Mombo number " + str(num))
    if num == 1:
        play = "tada.wav"
    elif num == 2:
        play = "Swing1.wav"
    elif num == 3:
        play = "Swing2.wav"
    elif num == 4:
        play = "Swing3.wav"
    elif num == 5:
        play = "listen.wav"

    with open(play, "rb") as f:
        f.read(44)  # Skip header
        while True:
            data = f.read(1024)
            if not data:
                break
            i2s.write(data)



def main():
    audioEN.on()
    time.sleep(0.01)

    playAudio(5)


    gpio17.on()   
    lights_state = 0
    # Your main code here
    print("Hello, MicroPython!")
    aw20072_init()
    while True:
        if not lights.value():  # Button pressed (active LOW)
            lights_state = not lights_state
            if lights_state:
                aw20x_MaxOn()
            else:
                aw20x_Off()
        # Debounce: wait for button release
            while not lights.value():
                time.sleep(0.01)
        time.sleep(0.01)

        x = acc.acceleration[0]
        y = acc.acceleration[1]
        z = acc.acceleration[2]

        # Calculate magnitude of acceleration vector
        magnitude = math.sqrt(x*x + y*y + z*z)

        print("Magnitude is " + str(magnitude))

        if magnitude > SWING_THRESHOLD:
            print("Found Swing")
            playAudio(random.randint(2,3))
            time.sleep(0.5)
            


if __name__ == "__main__":
    main()

