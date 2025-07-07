from machine import i2c, pin
import time

# setup i2c
i2c = i2c(0, scl=pin(10), sda=Pin(9), freq=100000)
aw_addr = 0x3a

# setup gpio 17
gpio17 = pin(17, pin.out)

# --- basic read/write helpers ---
def aw_write(reg, data):
   i2c.writeto_mem(aw_addr, reg, bytes([data]))

def aw_read(reg):
   return i2c.readfrom_mem(aw_addr, reg, 1)[0]

# --- init sequence ---
def aw_init():
   print("going through init")
   # 1. set gpio 17 high (assumed as "enable" or "not reset")
   gpio17.value(1)
   time.sleep(0.01)  # small delay just in case

   # make sure we are on the right page
   aw_write(0x0f, 0x00)

   # clear all LEDs
   aw_write(0x31, 0x00)
   aw_write(0x32, 0x00)
   aw_write(0x33, 0x00)
   aw_write(0x34, 0x00)
   aw_write(0x35, 0x00)
   aw_write(0x36, 0x00)
   aw_write(0x37, 0x00)
   aw_write(0x38, 0x00)
   aw_write(0x39, 0x00)
   aw_write(0x3A, 0x00)
   aw_write(0x3B, 0x00)
   aw_write(0x3C, 0x00)
   print("All LEDs cleared")

   # set size
   aw_write(0x80, 0x02)
   print("set size")

   # Wake from sleep
   aw_write(0x01, 0x00)
   print("Wake up!")

   aw_write(0x02, 0xC0)
   print("Set Amperage")

def lettherebe():
   # all LEDs on
   aw_write(0x31, 0x1F)
   aw_write(0x32, 0x1F)
   aw_write(0x33, 0x1F)
   aw_write(0x34, 0x1F)
   aw_write(0x35, 0x1F)
   aw_write(0x36, 0x00)
   aw_write(0x37, 0x00)
   aw_write(0x38, 0x00)
   aw_write(0x39, 0x00)
   aw_write(0x3A, 0x00)
   aw_write(0x3B, 0x00)
   aw_write(0x3C, 0x00)
   print("All LEDs cleared")


