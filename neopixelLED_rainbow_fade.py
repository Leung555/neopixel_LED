import time
import board
import neopixel_spi as neopixel
import math

NUM_PIXELS = 7
PIXEL_ORDER = neopixel.GRB
#COLORS = (0xFF0000, 0x00FF00, 0x0000FF)
DELAY = 0.05

spi = board.SPI()


# Choose an open pin connected to the Data In of the NeoPixel strip, i.e. board.D18
# NeoPixels must be connected to D10, D12, D18 or D21 to work.
#pixel_pin = board.D18

# The number of NeoPixels
num_pixels = 7

# The order of the pixel colors - RGB or GRB. Some NeoPixels have red and green reversed!
# For RGBW NeoPixels, simply change the ORDER to RGBW or GRBW.
ORDER = neopixel.GRB

pixels = neopixel.NeoPixel_SPI(spi,
                               NUM_PIXELS,
                               pixel_order=PIXEL_ORDER,
                               auto_write=False,
                brightness=0.5)


while True:
    wait = 0.01
    deg = 0
    degToRad = 3.14/180
    i = 0
    j = 120
    k = 240

    while(True):
        r = round((math.cos(i*degToRad)+1)*255//2)
        g = round((math.cos(j*degToRad)+1)*255//2)
        b = round((math.cos(k*degToRad)+1)*255//2)

        pixels.fill([r,g,b])
        pixels.show()
        time.sleep(DELAY)

        i += 1
        j += 1
        k += 1
        
        if i > 359:
            i = 0
        if j > 359:
            j=0
        if k > 359:
            k=0 
        #print("i: ", i)
        #print("r: ", r)
