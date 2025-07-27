# MicroPython program for Raspberry Pi Pico W and 16x48 NeoPixel matrix
# Displays the current time fetched via WiFi using NTP.

import time
import ntptime
import network
from machine import Pin
import neopixel

# WiFi credentials - update these for your network
WIFI_SSID = "your-ssid"
WIFI_PASSWORD = "your-password"

# Matrix configuration
MATRIX_WIDTH = 48
MATRIX_HEIGHT = 16
NUM_PIXELS = MATRIX_WIDTH * MATRIX_HEIGHT
DATA_PIN = 0  # GP0 on the Pico W

# Simple 5x3 font for digits 0-9 and colon
FONT = {
    '0': [0b11111,
          0b10001,
          0b10001,
          0b10001,
          0b11111],
    '1': [0b00100,
          0b01100,
          0b00100,
          0b00100,
          0b01110],
    '2': [0b11111,
          0b00001,
          0b11111,
          0b10000,
          0b11111],
    '3': [0b11111,
          0b00001,
          0b01110,
          0b00001,
          0b11111],
    '4': [0b10001,
          0b10001,
          0b11111,
          0b00001,
          0b00001],
    '5': [0b11111,
          0b10000,
          0b11111,
          0b00001,
          0b11111],
    '6': [0b11111,
          0b10000,
          0b11111,
          0b10001,
          0b11111],
    '7': [0b11111,
          0b00001,
          0b00010,
          0b00100,
          0b00100],
    '8': [0b11111,
          0b10001,
          0b11111,
          0b10001,
          0b11111],
    '9': [0b11111,
          0b10001,
          0b11111,
          0b00001,
          0b11111],
    ':': [0b00000,
          0b00100,
          0b00000,
          0b00100,
          0b00000],
}


class Matrix:
    """Driver for a 16x48 NeoPixel matrix."""

    def __init__(self, pin, width, height):
        self.width = width
        self.height = height
        self.np = neopixel.NeoPixel(Pin(pin, Pin.OUT), width * height)

    def _pixel_index(self, x, y):
        if y % 2 == 0:
            return y * self.width + x
        else:
            return y * self.width + (self.width - 1 - x)

    def fill(self, color):
        for i in range(self.np.n):
            self.np[i] = color
        self.np.write()

    def set_pixel(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.np[self._pixel_index(x, y)] = color

    def show(self):
        self.np.write()


matrix = Matrix(DATA_PIN, MATRIX_WIDTH, MATRIX_HEIGHT)


def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PASSWORD)
    while not wlan.isconnected():
        time.sleep(0.5)
    print("WiFi connected", wlan.ifconfig())


def sync_time():
    try:
        ntptime.settime()
    except Exception as e:
        print("NTP sync failed:", e)


def draw_char(char, x, y, color):
    bitmap = FONT.get(char)
    if not bitmap:
        return
    for row, bits in enumerate(bitmap):
        for col in range(5):
            if bits & (1 << (4 - col)):
                matrix.set_pixel(x + col, y + row, color)


def draw_text(text, x, y, color):
    for char in text:
        draw_char(char, x, y, color)
        x += 6  # 5 pixels + 1 space


connect_wifi()
sync_time()

while True:
    t = time.localtime()
    time_str = "{:02d}:{:02d}".format(t[3], t[4])
    matrix.fill((0, 0, 0))
    draw_text(time_str, 2, 4, (0, 20, 50))
    matrix.show()
    time.sleep(0.5)
